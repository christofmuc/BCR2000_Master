/*
   Copyright (c) 2019 Christof Ruch. All rights reserved.

   Dual licensed: Distributed under Affero GPL license by default, an MIT license is available for purchase
*/

#pragma once

#include "JuceHeader.h"

#include "BCR2000.h"
#include "AutoDetection.h"

#include "LambdaButtonStrip.h"

class BCLEditor : public Component,
	public ApplicationCommandTarget,
	private CodeDocument::Listener,
	private Timer
{
public:
	BCLEditor(std::shared_ptr<midikraft::BCR2000> bcr, std::function<void()> detectedHandler);
	virtual ~BCLEditor();

	virtual void resized() override;
	
	void loadDocument(std::string const &document);
	void loadDocumentFromSyx(std::vector<MidiMessage> const &messages);

	// Code document listener
	virtual void codeDocumentTextInserted(const String& newText, int insertIndex) override;
	virtual void codeDocumentTextDeleted(int startIndex, int endIndex) override;

	virtual ApplicationCommandTarget* getNextCommandTarget() override;
	virtual void getAllCommands(Array<CommandID>& commands) override;
	virtual void getCommandInfo(CommandID commandID, ApplicationCommandInfo& result) override;
	virtual bool perform(const InvocationInfo& info) override;

	// This is only to grab focus once
	virtual void timerCallback() override;

private:
	void loadDocument();
	void saveDocument();
	void saveAsDocument();

	void sendToBCR();

	void aboutBox();

	std::shared_ptr<midikraft::BCR2000> bcr_;
	std::function<void()> detectedHandler_;
	midikraft::AutoDetection autodetector_;
	std::unique_ptr<CodeEditorComponent> editor_;
	CodeDocument document_;
	LambdaButtonStrip buttons_;
	TextEditor currentError_;
	StringArray errors_;
	TextEditor helpText_;
	Label stdErrLabel_;

	ApplicationCommandManager commandManager_;
	String currentFilePath_;
	bool grabbedFocus_;
};

