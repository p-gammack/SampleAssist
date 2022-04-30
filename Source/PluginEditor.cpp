/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AutoSamplerAudioProcessorEditor::AutoSamplerAudioProcessorEditor (AutoSamplerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    startTimer(1);
    
    iDynIndex = 0;
    iNoteIndex = 0;
    
    setResizable(true, false);
    setSize (600, 400);
    setResizeLimits(600, 400, 600, 400);
    
    chooseDirectory();

    recordState = RECORDING_OFF;
//    addAndMakeVisible(&recordButton);
    recordButton.setButtonText("Record");
    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    recordButton.onClick = [this] { recordButtonClicked(); };
    recordButton.setEnabled(false);
    
    runState = SET;
    addAndMakeVisible(&runButton);
    runButton.setButtonText("Start");
    runButton.setColour(juce::TextButton::buttonColourId, colourButton);
    runButton.onClick = [this] { runButtonClicked(); };
    runButton.setEnabled(false);
    
    addAndMakeVisible(&nextNoteButton);
    nextNoteButton.setButtonText("Next Sample");
    nextNoteButton.setColour(juce::TextButton::buttonColourId, colourButton);
    nextNoteButton.onClick = [this] { nextNoteButtonClicked(); };
    nextNoteButton.setEnabled(false);
    
    addAndMakeVisible(&resetNoteButton);
    resetNoteButton.setButtonText("Reset Sample");
    resetNoteButton.setColour(juce::TextButton::buttonColourId, colourButton);
    resetNoteButton.onClick = [this] { resetNoteButtonClicked(); };
    resetNoteButton.setEnabled(false);
    
//    addAndMakeVisible(&restartButton);
    restartButton.setButtonText("Restart");
    restartButton.setColour(juce::TextButton::buttonColourId, colourButton);
    restartButton.setEnabled(false);
    
    addAndMakeVisible(&sampleSelection);
    sampleSelection.setText("Select Sample");
    sampleSelection.setColour(juce::ComboBox::backgroundColourId, colourButton);
    sampleSelection.onChange = [this] { sampleSelectionChanged(); };
    sampleSelection.setEnabled(false);
    for (int i=0; i<36; i++)
        sampleSelection.addItem(audioProcessor.sampleName[i], i+1);
    
    addAndMakeVisible(&p.waveform);
    p.waveform.setBounds(waveBox);
    p.waveform.setColours(colourBox, colourAccent1);
    p.waveform.setRepaintRate(60);
    
    addAndMakeVisible(&infoText[0]);
    infoText[0].setBoundingBox(infoTextBox[0]);
    infoText[0].setJustification(juce::Justification::centred);
    infoText[0].setFontHeight(25.0f);
    infoText[0].setColour(colourAccent1);
    infoText[0].setText("");
    
    addAndMakeVisible(&infoText[1]);
    infoText[1].setBoundingBox(infoTextBox[1]);
    infoText[1].setJustification(juce::Justification::centred);
    infoText[1].setFontHeight(25.0f);
    infoText[1].setColour(colourAccent1);
    infoText[1].setText("4");
    
    addAndMakeVisible(&infoText[2]);
    infoText[2].setBoundingBox(infoTextBox[2]);
    infoText[2].setJustification(juce::Justification::centred);
    infoText[2].setFontHeight(25.0f);
    infoText[2].setColour(colourAccent1);
    infoText[2].setText(audioProcessor.sampleName[0]);
}

AutoSamplerAudioProcessorEditor::~AutoSamplerAudioProcessorEditor()
{
}

//==============================================================================
void AutoSamplerAudioProcessorEditor::paint (juce::Graphics& g)
{
//    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.fillAll(colourBackground);
    g.setColour(colourBox);
    g.fillRect(waveBox);
    g.fillRect(infoBox);
//    for (int i=0;i<4;i++)
//        g.fillRect(infoTextBox[i]);
//    g.setColour(colourAccent1);
//    g.setFont(20.0f);
    g.setColour(juce::Colours::black);
    g.drawLine(recordLine, 1.0f);
//    g.drawRect(waveBox);
    
    g.setColour (juce::Colours::black);
    g.setFont (15.0f);
    g.drawFittedText ("SampleAssist by Patrick Gammack", getLocalBounds(), juce::Justification::centredBottom, 1);
}

void AutoSamplerAudioProcessorEditor::resized()
{
    int iWindowWidth = getWidth();
    int iWindowHeight = getHeight();
    int iMargin = 15;
    
    waveBox.setBounds(iMargin, iWindowHeight*0.5f - iMargin, iWindowWidth - (iMargin*2), iWindowHeight*0.5f );
    infoBox.setLeft(iWindowWidth*0.5f + iMargin);
    infoBox.setRight(iWindowWidth - iMargin);
    infoBox.setTop(iMargin);
    infoBox.setBottom((iWindowHeight*0.5f) - (iMargin*2));
    infoTextBox[0].setBounds(infoBox.getX(), infoBox.getY(), infoBox.getWidth(), infoBox.getHeight()*0.25f);
    for (int i=1;i<4;i++)
        infoTextBox[i].setBounds(infoBox.getX(), infoTextBox[i-1].getBottom(), infoBox.getWidth(), infoTextBox[i-1].getHeight());
    recordLine.setStart(waveBox.getCentreX(), waveBox.getY());
    recordLine.setEnd(waveBox.getCentreX(), waveBox.getBottom());
    recordButton.setBounds(30, 30, iWindowWidth - 60, 30);
    runButton.setBounds(iMargin, iMargin, iWindowWidth*0.5f - iMargin, (iWindowHeight*0.5f*0.25f) - (iMargin*0.5f) - iMargin);
    nextNoteButton.setBounds(runButton.getX(), runButton.getY() + runButton.getHeight() + iMargin, runButton.getWidth(), runButton.getHeight());
    resetNoteButton.setBounds(nextNoteButton.getX(), nextNoteButton.getY() + nextNoteButton.getHeight() + iMargin, nextNoteButton.getWidth(), nextNoteButton.getHeight());
    restartButton.setBounds(resetNoteButton.getX(), resetNoteButton.getY() + resetNoteButton.getHeight() + iMargin, resetNoteButton.getWidth(), resetNoteButton.getHeight());
    sampleSelection.setBounds(resetNoteButton.getX(), resetNoteButton.getY() + resetNoteButton.getHeight() + iMargin, resetNoteButton.getWidth(), resetNoteButton.getHeight());
    timer.setBoundingBox(infoTextBox[0]);
}

void AutoSamplerAudioProcessorEditor::timerCallback()
{

    if (runState == RUNNING && audioProcessor.iCount >= 0) {
        for (int i=0; i<=4; i++)
            if (audioProcessor.iCount == i)
                infoText[1].setText(std::to_string(i));
        if (audioProcessor.iCount <= 0)
            audioProcessor.iCount = -1;
    }
}

void AutoSamplerAudioProcessorEditor::recordButtonClicked()
{
    switch (recordState) {
        case RECORDING_OFF:
            recordState = RECORD_ARMED;
            audioProcessor.armRecording();
            break;
        case RECORD_ARMED:
            recordState = RECORDING_OFF;
            audioProcessor.stopRecording();
            break;
        case RECORDING:
            recordState = RECORDING_OFF;
            audioProcessor.stopRecording();
            break;
    }
}

void AutoSamplerAudioProcessorEditor::runButtonClicked()
{
    recordButtonClicked();
    
    switch (runState) {
        case SET:
            runState = RUNNING;
            runButton.setButtonText("Stop");
            nextNoteButton.setEnabled(true);
            resetNoteButton.setEnabled(true);
            sampleSelection.setEnabled(false);
            break;
        case RUNNING:
            runState = PAUSED;
            runButton.setButtonText("Start");
            runButton.setEnabled(false);
            sampleSelection.setEnabled(true);
            break;
        case PAUSED:
            runState = RUNNING;
            runButton.setButtonText("Stop");
            runButton.setEnabled(true);
            sampleSelection.setEnabled(false);
            break;
    }
}

void AutoSamplerAudioProcessorEditor::nextNoteButtonClicked()
{
    if (runState == RUNNING)
        runButtonClicked();
    if (runState == PAUSED)
        runButton.setEnabled(true);
    
    infoText[1].setText("4");

    if (audioProcessor.iSampleIndex < 35) {
        audioProcessor.iSampleIndex++;
        infoText[2].setText(audioProcessor.sampleName[audioProcessor.iSampleIndex]);
    }
    else
        nextNoteButton.setEnabled(false);
}

void AutoSamplerAudioProcessorEditor::resetNoteButtonClicked()
{
    if (runState == RUNNING) {
        runButtonClicked();
        runButtonClicked();
    }
    if (runState == PAUSED) {
        runButton.setEnabled(true);
        infoText[1].setText("4");
    }
}

void AutoSamplerAudioProcessorEditor::sampleSelectionChanged()
{
    if (sampleSelection.getSelectedId()) {
        printf("sampleSelectChanged() id: %i\n", sampleSelection.getSelectedId());
        audioProcessor.iSampleIndex = sampleSelection.getSelectedId() - 1;
        infoText[2].setText(audioProcessor.sampleName[audioProcessor.iSampleIndex]);
        
        if (audioProcessor.iSampleIndex >= 35)
            nextNoteButton.setEnabled(false);
        else if (!nextNoteButton.isEnabled())
            nextNoteButton.setEnabled(true);
        
        if (!runButton.isEnabled())
            runButton.setEnabled(true);
    }
    sampleSelection.setText("Select Sample");
}

void AutoSamplerAudioProcessorEditor::chooseDirectory()
{
    directoryChooser = std::make_unique<juce::FileChooser> ("Select a location to save samples...",
                                                            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                                                            "",
                                                            true);
    
    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;
    
    directoryChooser->launchAsync(chooserFlags, [this] (const juce::FileChooser& fc) {
        
        auto file = fc.getResult();
        
        if (file != juce::File{}) {
            audioProcessor.sampleDirectory = file.getFullPathName();
            runButton.setEnabled(true);
            nextNoteButton.setEnabled(true);
            sampleSelection.setEnabled(true);
        }
    });
}
