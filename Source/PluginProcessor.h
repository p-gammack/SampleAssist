/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/

class AutoSamplerAudioProcessor : public juce::AudioProcessor, public juce::ChangeListener
{
public:
    enum RunState {
        RUNNING,
        NOT_RUNNING,
    };
    enum RecordState {
        RECORDING_OFF,
        RECORD_ARMED,
        RECORDING,
    };
    
    juce::String sampleDirectory;
    
    std::string sampleName [36];
    
    int iSampleIndex;
    int iCount;
    
    //==============================================================================
    AutoSamplerAudioProcessor();
    ~AutoSamplerAudioProcessor() override;
    void changeListenerCallback (juce::ChangeBroadcaster* source) override {
        
    }

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    //==============================================================================
    void armRecording();
    void startRecording();
    void stopRecording();

    juce::AudioVisualiserComponent waveform;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoSamplerAudioProcessor)
    
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    
    std::unique_ptr<juce::AudioFormatWriter> audioWriter;
    
    juce::TimeSliceThread recordThread { "Audio Recorder Thread" };
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;
    juce::CriticalSection writerLock;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter { nullptr };
    
    std::string dynamicLayers [3];
    std::string notes [12];
    
    RunState runState;
    RecordState recordState;
    juce::File outputFile;
    int iFileNo;

    int iCountDown;
    int iBufferSize;
    int iSample;
    double dSampleRate;
    
    std::vector<int> iTimeStamps;
};
