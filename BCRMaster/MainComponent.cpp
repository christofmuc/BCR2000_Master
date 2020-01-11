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
	editor_(bcr_, [this]() { refreshListOfPresets();  }), 
	grid_(4, 8, [this](int no) { retrievePatch(no); })
{
	logger_ = std::make_unique<LogViewLogger>(logView_);
	addAndMakeVisible(multiDocs_);
	auto docWindow = new MultiBCRDDocumentWindow(editor_);
	multiDocs_.addDocument(docWindow, Colours::slategrey, true);
	addAndMakeVisible(docWindow);	
	addAndMakeVisible(logView_);
	addAndMakeVisible(grid_);
	addAndMakeVisible(midiLogView_);

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
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
	auto area = getLocalBounds();	
	logView_.setBounds(area.removeFromBottom(200).reduced(10));
	auto leftHalf = area.removeFromLeft(area.getWidth() * 4 / 5);
	multiDocs_.setBounds(leftHalf.reduced(10));
	grid_.setBounds(area.removeFromTop(area.getHeight() / 2));
	midiLogView_.setBounds(area.reduced(10));
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
	midikraft::MidiController::instance()->addMessageHandler(handle, [this, handle](MidiInput *source, MidiMessage const &message) {
		if (bcr_->isPartOfDump(message)) {
			currentDownload_.push_back(message);
		}
		if (bcr_->isDumpFinished(currentDownload_)) {
			midikraft::MidiController::instance()->removeMessageHandler(handle);
			editor_.loadDocumentFromSyx(currentDownload_);
		}
	});
	midikraft::MidiController::instance()->getMidiOutput(bcr_->midiOutput())->sendMessageNow(bcr_->requestDump(no));
}

bool MultiBCRDocuments::tryToCloseDocument(Component* component)
{
	return true;
}
