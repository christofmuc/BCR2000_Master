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
#include "AutoDetection.h"

class LogViewLogger;

class BCRMenu : public MenuBarModel {
public:
	BCRMenu(ApplicationCommandManager *commandManager, LambdaButtonStrip *lambdaButtons);

	StringArray getMenuBarNames() override;
	PopupMenu getMenuForIndex(int topLevelMenuIndex, const String& menuName) override;
	void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

private:
	std::map<int, std::pair<std::string, std::vector<std::string>>> menuStructure_;
	ApplicationCommandManager *commandManager_;
	LambdaButtonStrip *lambdaButtons_;
};

class MainComponent : public Component, public ApplicationCommandTarget
{
public:
    MainComponent();
    ~MainComponent();

    void resized() override;

	void refreshListOfPresets();

	// Required to process commands. That seems a bit heavyweight
	ApplicationCommandTarget* getNextCommandTarget() override;
	void getAllCommands(Array<CommandID>& commands) override;
	void getCommandInfo(CommandID commandID, ApplicationCommandInfo& result) override;
	bool perform(const InvocationInfo& info) override;

private:
	void retrievePatch(int no);
	BCLEditor *createNewEditor(std::string const &tabName);
	void addNewEditor(std::string const &tabName, BCLEditor *editor);
	BCLEditor *activeTab();

	void aboutBox();

	midikraft::AutoDetection autodetector_;
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

	LambdaButtonStrip buttons_;
	ApplicationCommandManager commandManager_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
