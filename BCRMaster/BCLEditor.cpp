#include "..\juce-widgets\SimpleTable.h"
#include "BCLEditor.h"

#include "StreamLogger.h"
#include "MidiController.h"

#include "Sysex.h"
#include "BCR2000.h"

#include "AutoDetection.h"

#include "Settings.h"

#include <memory>
#include <sstream>

const char *kLastPath = "LastDocumentPath";

template <>
void visit(midikraft::BCR2000::BCRError const &errorStruct, int column, std::function<void(std::string const &)> visitor) {
	switch (column) {
	case 1: visitor(String(errorStruct.lineNumber).toStdString()); break;
	case 2: visitor(String(errorStruct.errorCode).toStdString()); break;
	case 3: visitor(errorStruct.errorText); break;
	case 4: visitor(errorStruct.lineText); break;
	}
}

BCLEditor::BCLEditor(std::shared_ptr<midikraft::BCR2000> bcr, std::function<void()> detectedHandler) : bcr_(bcr), detectedHandler_(detectedHandler),
	buttons_(201, LambdaButtonStrip::Direction::Horizontal), grabbedFocus_(false), currentError_({ "Line", "Error code", "Error description", "Text" }, { })
{
	editor_ = std::make_unique<CodeEditorComponent>(document_, nullptr);
	addAndMakeVisible(editor_.get());
	LambdaButtonStrip::TButtonMap buttons = {
		{ "connect", {0, "Connect to BCR2000", [this]() {
			MouseCursor::showWaitCursor();
			std::vector<std::shared_ptr<midikraft::SimpleDiscoverableDevice>> devices;
			devices.push_back(bcr_);
			autodetector_.autoconfigure(devices);
			bcr_->invalidateListOfPresets();
			bcr_->refreshListOfPresets([this]() {
				// Back to the UI thread please
				MessageManager::callAsync([this]() {
					detectedHandler_();
					MouseCursor::hideWaitCursor();
				});
			});
			
		}}},
		{ "load", { 1, "Open (CTRL-O)", [this]() {
			loadDocument();
			editor_->grabKeyboardFocus();
		}, 0x4F /* O */, ModifierKeys::ctrlModifier}},
		{ "save", { 2, "Save (CTRL-S)", [this]() {
			saveDocument();
		}, 0x53 /* S */, ModifierKeys::ctrlModifier}},
		{ "saveAs", { 3, "Save as (CTRL-A)", [this]() {
			saveAsDocument();
		}, 0x41 /* A */, ModifierKeys::ctrlModifier}},
		{ "send", { 4, "Send to BCR", [this]() {
			sendToBCR();
		}, 0x41 /* A */, ModifierKeys::ctrlModifier}},
		{ "about", { 5, "About", [this]() {
			aboutBox();
		}, -1, 0}},
		{ "close", { 6, "Close (CTRL-W)", []() {
			JUCEApplicationBase::quit();
		}, 0x57 /* W */, ModifierKeys::ctrlModifier}}
	};
	buttons_.setButtonDefinitions(buttons);
	addAndMakeVisible(buttons_);
	addAndMakeVisible(helpText_);
	addAndMakeVisible(stdErrLabel_);
	addAndMakeVisible(currentError_);
	stdErrLabel_.setText("stderr:", dontSendNotification);
	document_.addListener(this);

	helpText_.setReadOnly(true);
	helpText_.setMultiLine(true, false);
	helpText_.setText("Welcome to the PyTschirp demo program. Below is a python script editor which already imported pytschirp.\n"
		"\n"
		"Type some commands like 'r = Rev2()' and press CTRL-ENTER to execute the script");	

	// Setup hot keys
	commandManager_.registerAllCommandsForTarget(&buttons_);
	addKeyListener(commandManager_.getKeyMappings());
	editor_->setCommandManager(&commandManager_);

	editor_->setWantsKeyboardFocus(true);
	startTimer(100);
}

BCLEditor::~BCLEditor()
{
	document_.removeListener(this);
}

void BCLEditor::resized()
{
	Rectangle<int> area(getLocalBounds());

	buttons_.setBounds(area.removeFromTop(60).reduced(20));
	helpText_.setBounds(area.removeFromTop(60).withTrimmedLeft(20).withTrimmedRight(20));

	auto stdErr = area.removeFromBottom(200);
	stdErrLabel_.setBounds(stdErr.removeFromTop(20));
	currentError_.setBounds(stdErr);

	editor_->setBounds(area.reduced(20));
}

void BCLEditor::loadDocument(std::string const &document)
{
	editor_->loadContent(document);
}

void BCLEditor::loadDocument()
{
	std::string lastPath = Settings::instance().get(kLastPath, File::getSpecialLocation(File::userDocumentsDirectory).getFullPathName().toStdString());
	FileChooser chooser("Please select the BCR2000 preset file to load...",
		File(lastPath),
		"*.syx;*.bcr;*.bcl");

	if (chooser.browseForFileToOpen())
	{
		File bclFile(chooser.getResult());
		Settings::instance().set(kLastPath, bclFile.getParentDirectory().getFullPathName().toStdString());
		if (!bclFile.existsAsFile()) {
			return;
		}
		currentFilePath_ = bclFile.getFullPathName();
		if (bclFile.getFileExtension() == ".syx") {
			auto messages = Sysex::loadSysex(bclFile.getFullPathName().toStdString());
			loadDocumentFromSyx(messages);
		}
		else {
			editor_->loadContent(bclFile.loadFileAsString());
		}
	}
}

void BCLEditor::loadDocumentFromSyx(std::vector<MidiMessage> const &messages)
{
	std::stringstream result;
	for (const auto& message : messages) {
		if (midikraft::BCR2000::isSysexFromBCR2000(message)) {
			result << midikraft::BCR2000::convertSyxToText(message) << std::endl;
		}
	}
	editor_->loadContent(result.str());
}

void BCLEditor::saveDocument()
{
	if (currentFilePath_.isNotEmpty()) {
		File bclFile(currentFilePath_);
		if (bclFile.existsAsFile() && bclFile.hasWriteAccess()) {
			bclFile.deleteFile();
		}
		FileOutputStream out(bclFile);
		if (out.openedOk()) {
			out.writeText(document_.getAllContent(), false, false, nullptr);
		}
	}
	else {
		saveAsDocument();
	}
}

void BCLEditor::saveAsDocument()
{
	FileChooser chooser("Save as...",
		File::getSpecialLocation(File::userHomeDirectory),
		"*.bcl");

	if (chooser.browseForFileToSave(true))
	{
		File chosenFile(chooser.getResult());
		currentFilePath_ = chosenFile.getFullPathName();
		saveDocument();
	}
}

void BCLEditor::sendToBCR()
{
	auto sysex = bcr_->convertToSyx(document_.getAllContent().toStdString());
	bcr_->sendSysExToBCR(midikraft::MidiController::instance()->getMidiOutput(bcr_->midiOutput()), sysex, SimpleLogger::instance(), [this](std::vector<midikraft::BCR2000::BCRError> const &errors) {
		bcr_->invalidateListOfPresets();
		currentError_.updateData(errors);
	});
}

void BCLEditor::aboutBox()
{
	String message = "This software is copyright 2020 by Christof Ruch\n"
		"Released under dual license, by default under AGPL-3.0, but an MIT licensed version is available on request by the author\n"
		"\n"
		"This software is provided 'as-is,' without any express or implied warranty.In no event shall the author be held liable for any damages arising from the use of this software.\n"
		"\n"
		"Other licenses:\n"
		"This software is build using JUCE, who might want to track your IP address. See https://github.com/WeAreROLI/JUCE/blob/develop/LICENSE.md for details.\n"
		"The boost library is used for parts of this software, see https://www.boost.org/.\n"
		"The installer provided also contains the Microsoft Visual Studio 2017 Redistributable Package.\n"
		;
	AlertWindow::showMessageBox(AlertWindow::InfoIcon, "About", message, "Close");
}

void BCLEditor::codeDocumentTextInserted(const String& newText, int insertIndex)
{
}

void BCLEditor::codeDocumentTextDeleted(int startIndex, int endIndex)
{
}

juce::ApplicationCommandTarget* BCLEditor::getNextCommandTarget()
{
	// Delegate to the lambda button strip
	return &buttons_;
}

void BCLEditor::getAllCommands(Array<CommandID>& commands)
{
	// Editor itself has no commands, this is only used to delegate commands the CodeEditor does not handle to the lambda button strip
}

void BCLEditor::getCommandInfo(CommandID commandID, ApplicationCommandInfo& result)
{
	// None, as no commands are registered here
}

bool BCLEditor::perform(const InvocationInfo& info)
{
	// Always false, as no commands are registered here
	return false;
}

void BCLEditor::timerCallback()
{
	if (editor_->isShowing()) {
		editor_->grabKeyboardFocus();
		grabbedFocus_ = true;
		stopTimer();
	}
}

