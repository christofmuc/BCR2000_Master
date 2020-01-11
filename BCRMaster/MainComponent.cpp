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
	topArea_(&tabs_, &grid_, -0.7, -0.3)
{
	logger_ = std::make_unique<LogViewLogger>(logView_);
	addAndMakeVisible(resizerBar_);
	addAndMakeVisible(logArea_);
	addAndMakeVisible(topArea_);
	createNewEditor("New");

	// Resizer bar allows to enlarge the log area
	stretchableManager_.setItemLayout(0, -0.1, -0.9, -0.8); // The editor tab window prefers to get 80%
	stretchableManager_.setItemLayout(1, 5, 5, 5);  // The resizer is hard-coded to 5 pixels
	stretchableManager_.setItemLayout(2, -0.1, -0.9,  -0.2);

	// Install our MidiLogger
	midikraft::MidiController::instance()->setMidiLogFunction([this](const MidiMessage& message, const String& source, bool isOut) {
		midiLogView_.addMessageToList(message, source, isOut);
	});

    // Make sure you set the size of the component after
    // you add any child components.
    setSize (1280, 800);
}

MainComponent::~MainComponent()
{
	Logger::setCurrentLogger(nullptr);
}

void MainComponent::resized()
{
	auto area = getLocalBounds();	

	// make a list of two of our child components that we want to reposition
	Component* comps[] = { &topArea_, &resizerBar_, &logArea_};

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
	editors_.add(editor);
	tabs_.addTab(tabName, getLookAndFeel().findColour(Label::backgroundColourId), editor, false);
	tabs_.addAndMakeVisible(editor);
	return editor;
}
