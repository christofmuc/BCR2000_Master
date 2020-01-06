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

class LogViewLogger;

class MainComponent   : public Component
{
public:
    MainComponent();
    ~MainComponent();

    void resized() override;

	void refreshListOfPresets();

private:
	void retrievePatch(int no);

	std::shared_ptr<midikraft::BCR2000> bcr_;
	BCLEditor editor_;
	LogView logView_;
	PatchButtonGrid grid_;
	MidiLogView midiLogView_;
	std::unique_ptr<LogViewLogger> logger_;
	std::vector<MidiMessage> currentDownload_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
