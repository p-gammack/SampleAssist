/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/

class ColourPalette
{
public:
    ColourPalette() {
        background = juce::Colour(51,63,80);
        box = juce::Colour(33,40,52);
        accent0 = juce::Colour(220,0,0);
    }
    ~ColourPalette();
private:
    juce::Colour background;
    juce::Colour box;
    juce::Colour accent0;
};

class MyTimer :
public juce::Timer
{
public:
    MyTimer() {
        second = 0;
        minute = 0;
        hour = 0;
        
        display.setColour(juce::Colour(255,177,0));
        display.setJustification(juce::Justification::centred);
        display.setFontHeight(25.0f);
        display.setText("0:0:0");
    }
    ~MyTimer() {}
    
    void setBoundingBox(juce::Parallelogram<float> newBounds) {
        display.setBoundingBox(newBounds);
    }
    void countUp(int hours, int minutes, int seconds) {
        second += seconds;
        minute += minutes;
        hour += hours;
        if (second > 59)
            minute++;
        if (minute > 59)
            hour++;
    }
    void timerCallback() {
        countUp(0, 0, getTimerInterval()*0.001f);
        display.setText(getTime());
    }
    std::string getTime() {
        std::string time = std::to_string(hour) + ":" + std::to_string(minute) + ":" + std::to_string(second);
        return time;
    }
private:
    int second;
    int minute;
    int hour;
    
    juce::DrawableText display;
};

//==============================================================================
/**
*/

class AutoSamplerAudioProcessorEditor :
public juce::AudioProcessorEditor,
private juce::Timer
{
public:
    enum RecordState {
        RECORDING_OFF,
        RECORD_ARMED,
        RECORDING,
    };
    enum RunState {
        SET,
        RUNNING,
        PAUSED,
    };
    
    //==============================================================================
    AutoSamplerAudioProcessorEditor (AutoSamplerAudioProcessor&);
    ~AutoSamplerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void recordButtonClicked();
    void runButtonClicked();
    void nextNoteButtonClicked();
    void resetNoteButtonClicked();
    void sampleSelectionChanged();
    void chooseDirectory();

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AutoSamplerAudioProcessor& audioProcessor;
    
    std::unique_ptr<juce::FileChooser> directoryChooser;
    
    MyTimer timer;

    RecordState recordState;
    RunState runState;
    
    int iDynIndex;
    int iNoteIndex;
    
    // COLOURS
    juce::Colour colourBackground = juce::Colour(28,28,28);
    juce::Colour colourBox = juce::Colour(45,45,45);
    juce::Colour colourAccent0 = juce::Colour(220,0,0);
    juce::Colour colourAccent1 = juce::Colour(255,177,0);
    juce::Colour colourButton = juce::Colour(191,191,191);

    // SHAPES
    juce::Rectangle<int> waveBox;
    juce::Rectangle<int> infoBox;
    juce::Rectangle<float> infoTextBox [4];
    juce::Line<float> recordLine;
    
    // BUTTONS
    juce::TextButton recordButton;
    juce::TextButton runButton;
    juce::TextButton nextNoteButton;
    juce::TextButton resetNoteButton;
    juce::TextButton restartButton;
    juce::ComboBox sampleSelection;
    
    // TEXT
    juce::DrawableText infoText [4];
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoSamplerAudioProcessorEditor)
};
