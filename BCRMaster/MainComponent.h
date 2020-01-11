/*
   Copyright (c) 2019 Christof Ruch. All rights reserved.

   Dual licensed: Distributed under Affero GPL license by default, an MIT license is available for purchase
*/

#pragma once

#include "JuceHeader.h"

#include "BCLEditor.h"
#include "BCR2000.h"
#include "LogView.h"
#include "MidiLogView.h"
#include "PatchButtonGrid.h"
#include "HorizontalLayoutContainer.h"

class LogViewLogger;

class BCRMenu : public MenuBarModel {
public:
	StringArray getMenuBarNames() override;
	PopupMenu getMenuForIndex(int topLevelMenuIndex, const String& menuName) override;
	void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;
};

class MainComponent   : public Component
{
public:
    MainComponent();
    ~MainComponent();

    void resized() override;

	void refreshListOfPresets();

private:
	void retrievePatch(int no);
	BCLEditor *createNewEditor(std::string const &tabName);

	std::shared_ptr<midikraft::BCR2000> bcr_;
	TabbedComponent tabs_;
	OwnedArray<BCLEditor> editors_;
	LogView logView_;
	PatchButtonGrid grid_;
	StretchableLayoutManager stretchableManager_;
	StretchableLayoutResizerBar resizerBar_;
	MidiLogView midiLogView_;
	std::unique_ptr<LogViewLogger> logger_;
	std::unique_ptr<BCRMenu> menu_;
	std::vector<MidiMessage> currentDownload_;
	MenuBarComponent menuBar_;

	HorizontalLayoutContainer topArea_;
	HorizontalLayoutContainer logArea_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
