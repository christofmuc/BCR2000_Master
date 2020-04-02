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
	grabbedFocus_(false),
	currentError_({ "Line", "Error code", "Error description", "Text" }, { }, [this](int rowSelected) {
		jumpToLine(rowSelected - 1);
		int errorRow = -1;
		if (rowSelected < lastErrors_.size()) {
			errorRow = lastErrors_[rowSelected].lineNumber;
		}
		editor_->selectRegion(CodeDocument::Position(document_, errorRow - 1, 0), CodeDocument::Position(document_, errorRow, 0));
	})
{
	editor_ = std::make_unique<CodeEditorComponent>(document_, nullptr);
	addAndMakeVisible(editor_.get());
	addAndMakeVisible(currentError_);
	document_.addListener(this);

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
	currentError_.setBounds(area.removeFromBottom(160).withTrimmedTop(8));
	editor_->setBounds(area);
}

void BCLEditor::loadDocument(std::string const &document)
{
	editor_->loadContent(document);
}

bool BCLEditor::loadDocument()
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
			return false;
		}
		currentFilePath_ = bclFile.getFullPathName();
		if (bclFile.getFileExtension() == ".syx") {
			auto messages = Sysex::loadSysex(bclFile.getFullPathName().toStdString());
			loadDocumentFromSyx(messages);
		}
		else {
			editor_->loadContent(bclFile.loadFileAsString());
		}
		return true;
	}
	return false;
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

void BCLEditor::jumpToLine(int rowNumber)
{
	editor_->scrollToLine(rowNumber);
}

void BCLEditor::saveDocument()
{
	if (currentFilePath_.isNotEmpty()) {
		File bclFile(currentFilePath_);
		if (bclFile.existsAsFile() && bclFile.hasWriteAccess()) {
			bclFile.deleteFile();
		}
		if (bclFile.getFileExtension().toLowerCase() == ".syx") {
			auto messages = bcr_->convertToSyx(document_.getAllContent().toStdString(), true);
			Sysex::saveSysex(bclFile.getFullPathName().toStdString(), messages);
		}
		else {
			// Write as ASCII text
			FileOutputStream out(bclFile);
			if (out.openedOk()) {
				out.writeText(document_.getAllContent(), false, false, nullptr);
			}
		}
	}
	else {
		saveAsDocument();
	}
}

void BCLEditor::saveAsDocument()
{
	std::string lastPath = Settings::instance().get(kLastPath, File::getSpecialLocation(File::userDocumentsDirectory).getFullPathName().toStdString());
	FileChooser chooser("Save as...",
		File(lastPath),
		"*.bcl||*.syx");

	if (chooser.browseForFileToSave(true))
	{
		File chosenFile(chooser.getResult());
		Settings::instance().set(kLastPath, chosenFile.getParentDirectory().getFullPathName().toStdString());
		currentFilePath_ = chosenFile.getFullPathName();
		saveDocument();
	}
}

void BCLEditor::sendToBCR()
{
	auto sysex = bcr_->convertToSyx(document_.getAllContent().toStdString(), true); // Make sure to set verbatim flag, otherwise the line numbers won't match
	bcr_->sendSysExToBCR(midikraft::MidiController::instance()->getMidiOutput(bcr_->midiOutput()), sysex, SimpleLogger::instance(), [this](std::vector<midikraft::BCR2000::BCRError> const &errors) {
		bcr_->invalidateListOfPresets();
		currentError_.updateData(errors);
		lastErrors_ = errors;
	});
}

juce::String BCLEditor::currentFileName() const
{
	return currentFilePath_;
}

void BCLEditor::codeDocumentTextInserted(const String& newText, int insertIndex)
{
}

void BCLEditor::codeDocumentTextDeleted(int startIndex, int endIndex)
{
}

void BCLEditor::timerCallback()
{
	if (editor_->isShowing()) {
		editor_->grabKeyboardFocus();
		grabbedFocus_ = true;
		stopTimer();
	}
}

