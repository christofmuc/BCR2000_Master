/*
   Copyright (c) 2019 Christof Ruch. All rights reserved.

   Dual licensed: Distributed under Affero GPL license by default, an MIT license is available for purchase
*/

#pragma once

#include "JuceHeader.h"

#include "BCR2000.h"

#include "SimpleTable.h"

class BCLEditor : public Component,
	private CodeDocument::Listener,
	private Timer
{
public:
	BCLEditor(std::shared_ptr<midikraft::BCR2000> bcr, std::function<void()> detectedHandler);
	virtual ~BCLEditor();

	virtual void resized() override;
	
	void loadDocument(std::string const &document);
	void loadDocumentFromSyx(std::vector<MidiMessage> const &messages);

	// Navigate document
	void jumpToLine(int rowNumber);

	// Code document listener
	virtual void codeDocumentTextInserted(const String& newText, int insertIndex) override;
	virtual void codeDocumentTextDeleted(int startIndex, int endIndex) override;

	// This is only to grab focus once
	virtual void timerCallback() override;

	bool loadDocument();
	void saveDocument();
	void saveAsDocument();
	void sendToBCR();

	String currentFileName() const;

private:
	std::shared_ptr<midikraft::BCR2000> bcr_;
	std::function<void()> detectedHandler_;	
	std::unique_ptr<CodeEditorComponent> editor_;
	CodeDocument document_;
	SimpleTable<std::vector<midikraft::BCR2000::BCRError>> currentError_;
	StringArray errors_;
	std::vector<midikraft::BCR2000::BCRError> lastErrors_;	

	String currentFilePath_;
	bool grabbedFocus_;
};

