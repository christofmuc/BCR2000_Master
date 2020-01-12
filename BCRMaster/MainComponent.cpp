/*
   Copyright (c) 2019 Christof Ruch. All rights reserved.

   Dual licensed: Distributed under Affero GPL license by default, an MIT license is available for purchase
*/

#include "MainComponent.h"

#include "Logger.h"
#include "MidiController.h"

class LogViewLogger : public SimpleLogger {
public:
	LogViewLogger(LogView &logview) : logview_(logview) {}


	virtual void postMessage(const String& message) override
	{
		logview_.addMessageToList(message);
	}

private:
	LogView &logview_;
};

//==============================================================================
MainComponent::MainComponent() : bcr_(std::make_shared<midikraft::BCR2000>()),
	tabs_(TabbedButtonBar::Orientation::TabsAtTop),
	grid_(4, 8, [this](int no) { retrievePatch(no); }),
	resizerBar_(&stretchableManager_, 1, false),
	logArea_(&logView_, &midiLogView_, -0.5, 0.5),
	topArea_(&tabs_, &grid_, -0.7, -0.3),
	buttons_(301, LambdaButtonStrip::Direction::Horizontal)
{
	LambdaButtonStrip::TButtonMap buttons = {
	{ "Detect", {0, "Connect to BCR2000", [this]() {
		MouseCursor::showWaitCursor();
		std::vector<std::shared_ptr<midikraft::SimpleDiscoverableDevice>> devices;
		devices.push_back(bcr_);
		//autodetector_.autoconfigure(devices);
		bcr_->invalidateListOfPresets();
		bcr_->refreshListOfPresets([this]() {
			// Back to the UI thread please
			MessageManager::callAsync([this]() {
				//detectedHandler_();
				MouseCursor::hideWaitCursor();
			});
		});

	}}},
	{ "Open", { 1, "Open", [this]() {
		auto active = createNewEditor("New");
		if (active->loadDocument()) {
			File loaded(active->currentFileName());
			addNewEditor(loaded.getFileNameWithoutExtension().toStdString(), active);
			tabs_.setCurrentTabIndex(tabs_.getNumTabs() - 1);
			//editor_->grabKeyboardFocus();
		}
		else {
			// User canceled
			delete active;
		}
	}, 0x4F /* O */, ModifierKeys::ctrlModifier}},
	{ "Save", { 2, "Save", [this]() {
		auto active = activeTab();
		if (active) {
			active->saveDocument();
		}
	}, 0x53 /* S */, ModifierKeys::ctrlModifier}},
	{ "Save as...", { 3, "Save as...", [this]() {
		auto active = activeTab();
		if (active) {
			active->saveAsDocument();
		}
	}, 0x41 /* A */, ModifierKeys::ctrlModifier}},
	{ "Send to BCR", { 4, "Send to BCR", [this]() {
		auto active = activeTab();
		if (active) {
			active->sendToBCR();
		}
	}, 0x41 /* A */, ModifierKeys::ctrlModifier}},
	{ "About", { 5, "About", [this]() {
		aboutBox();
	}, -1, 0}},
	{ "Quit", { 6, "Quit", []() {
		JUCEApplicationBase::quit();
	}, 0x57 /* W */, ModifierKeys::ctrlModifier}}
	};
	buttons_.setButtonDefinitions(buttons);
	commandManager_.registerAllCommandsForTarget(&buttons_);
	// Stop the focus-based search for the commands, and rather route all command implementation into this MainComponent
	// That was tricky to find out
	commandManager_.setFirstCommandTarget(this);
	addKeyListener(commandManager_.getKeyMappings());

	// Setup menu structure
	menu_ = std::make_unique<BCRMenu>(&commandManager_, &buttons_);
	menu_->setApplicationCommandManagerToWatch(&commandManager_);
	menuBar_.setModel(menu_.get());

	// Setup the rest of the UI
	logger_ = std::make_unique<LogViewLogger>(logView_);
	addAndMakeVisible(menuBar_);
	addAndMakeVisible(resizerBar_);
	addAndMakeVisible(logArea_);
	addAndMakeVisible(topArea_);
	auto newEditor = createNewEditor("New");
	addNewEditor("New", newEditor);

	// Resizer bar allows to enlarge the log area
	stretchableManager_.setItemLayout(0, -0.1, -0.9, -0.8); // The editor tab window prefers to get 80%
	stretchableManager_.setItemLayout(1, 5, 5, 5);  // The resizer is hard-coded to 5 pixels
	stretchableManager_.setItemLayout(2, -0.1, -0.9, -0.2);

	// Install our MidiLogger
	midikraft::MidiController::instance()->setMidiLogFunction([this](const MidiMessage& message, const String& source, bool isOut) {
		midiLogView_.addMessageToList(message, source, isOut);
	});

	// Make sure you set the size of the component after
	// you add any child components.
	setSize(1280, 800);
}

MainComponent::~MainComponent()
{
	Logger::setCurrentLogger(nullptr);
}

void MainComponent::resized()
{
	auto area = getLocalBounds();
	menuBar_.setBounds(area.removeFromTop(30));

	// make a list of two of our child components that we want to reposition
	Component* comps[] = { &topArea_, &resizerBar_, &logArea_ };

	// this will position the 3 components, one above the other, to fit
	// vertically into the rectangle provided.
	stretchableManager_.layOutComponents(comps, 3,
		area.getX(), area.getY(), area.getWidth(), area.getHeight(),
		true, true);
}

void MainComponent::refreshListOfPresets()
{
	int i = 0;
	for (auto const &preset : bcr_->listOfPresets()) {
		auto button = grid_.buttonWithIndex(i++);
		if (button) {
			//button->setActive(false);
			button->setButtonText(preset);
			//button->setToggleState(false);
			Colour color = Colours::aliceblue;
			//button->setColour(TextButton::ColourIds::buttonColourId, color.darker());
		}
	}
	repaint();
}

void MainComponent::retrievePatch(int no)
{
	//TODO not really thread safe or even frantic user safe...
	currentDownload_.clear();
	midikraft::MidiController::HandlerHandle handle = midikraft::MidiController::makeOneHandle();
	midikraft::MidiController::instance()->addMessageHandler(handle, [this, handle, no](MidiInput *source, MidiMessage const &message) {
		if (bcr_->isPartOfDump(message)) {
			currentDownload_.push_back(message);
		}
		if (bcr_->isDumpFinished(currentDownload_)) {
			midikraft::MidiController::instance()->removeMessageHandler(handle);
			MessageManager::callAsync([this, no]() {
				auto patchName = grid_.buttonWithIndex(no) ? grid_.buttonWithIndex(no)->getButtonText() : "unnamed";
				auto editor = createNewEditor(patchName.toStdString());
				addNewEditor(patchName.toStdString(), editor);
				editor->loadDocumentFromSyx(currentDownload_);
				tabs_.setCurrentTabIndex(tabs_.getNumTabs() - 1);
			});
		}
	});
	midikraft::MidiController::instance()->getMidiOutput(bcr_->midiOutput())->sendMessageNow(bcr_->requestDump(no));
}

BCLEditor *MainComponent::createNewEditor(std::string const &tabName)
{
	auto editor = new BCLEditor(bcr_, [this]() { refreshListOfPresets();  });
	return editor;
}

void MainComponent::addNewEditor(std::string const &tabName, BCLEditor *editor) {
	editors_.add(editor);
	tabs_.addTab(tabName, getLookAndFeel().findColour(Label::backgroundColourId), editor, false);
	tabs_.addAndMakeVisible(editor);
}

BCLEditor * MainComponent::activeTab()
{
	return dynamic_cast<BCLEditor *>(tabs_.getTabContentComponent(tabs_.getCurrentTabIndex()));
}

juce::ApplicationCommandTarget* MainComponent::getNextCommandTarget()
{
	// Delegate to the lambda button strip
	return &buttons_;
}

void MainComponent::getAllCommands(Array<CommandID>& commands)
{
	// Editor itself has no commands, this is only used to delegate commands the CodeEditor does not handle to the lambda button strip
}

void MainComponent::getCommandInfo(CommandID commandID, ApplicationCommandInfo& result)
{
	// None, as no commands are registered here
}

bool MainComponent::perform(const InvocationInfo& info)
{
	// Always false, as no commands are registered here
	return false;
}


BCRMenu::BCRMenu(ApplicationCommandManager *commandManager, LambdaButtonStrip *lambdaButtons) : 
	commandManager_(commandManager), lambdaButtons_(lambdaButtons)
{
	menuStructure_ = {
		{0, { "File", { "New", "Open", "Save", "Save as...", "Close", "Quit" } } },
		{1, { "BCR2000", { "Detect", "Refresh preset list", "Send to BCR" } } },
		{2, { "Help", { "About" } } }
	};
}

StringArray BCRMenu::getMenuBarNames()
{
	StringArray result;
	for (int i = 0; i < menuStructure_.size(); i++) {
		result.add(menuStructure_[i].first);
	}
	return result;
}

PopupMenu BCRMenu::getMenuForIndex(int topLevelMenuIndex, const String& menuName)
{
	PopupMenu menu;
	for (auto item : menuStructure_[topLevelMenuIndex].second) {
		// Ok, we want a command-based menu bar - for that, we need to search all commands in the lambdaButtons and fine one with the correct name
		Array<int> commands;
		lambdaButtons_->getAllCommands(commands);
		for (auto command : commands) {
			ApplicationCommandInfo info(command);
			lambdaButtons_->getCommandInfo(command, info);
			if (info.description == String(item)) {
				// Found!
				menu.addCommandItem(commandManager_, command);
				break;
			}
		}
	}
	return menu;
}

void BCRMenu::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
}

void MainComponent::aboutBox()
{
	String message = "This software is copyright 2020 by Christof Ruch\n\n"
		"Released under dual license, by default under AGPL-3.0, but an MIT licensed version is available on request by the author\n"
		"\n"
		"This software is provided 'as-is,' without any express or implied warranty. In no event shall the author be held liable for any damages arising from the use of this software.\n"
		"\n"
		"Other licenses:\n"
		"This software is build using JUCE, who might want to track your IP address. See https://github.com/WeAreROLI/JUCE/blob/develop/LICENSE.md for details.\n"
		"The boost library is used for parts of this software, see https://www.boost.org/.\n"
		"The installer provided also contains the Microsoft Visual Studio 2017 Redistributable Package.\n"
		;
	AlertWindow::showMessageBox(AlertWindow::InfoIcon, "About", message, "Close");
}

