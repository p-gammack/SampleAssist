/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AutoSamplerAudioProcessor::AutoSamplerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
, waveform(2), outputFile()
{
    std::string dynamicLayers [3] = {
        "p", "mf", "ff"
    };
    std::string notes [12] = {
        "C0", "G0", "D1", "A1", "E2", "B2", "F#3", "C#4", "G#4", "D#5", "A#5", "F6"
    };
    
    for (int d = 0; d < 3; d++)
        for (int n = 0; n < 12; n++)
            sampleName[d*12+n] = dynamicLayers[d] + "_" + notes[n];
    
    for (int i=0; i<36; i++)
        printf("%s\n", sampleName[i].c_str());
    
    runState = NOT_RUNNING;
    recordState = RECORDING_OFF;
    iSampleIndex = 0;
    dSampleRate = 0;
    iSample = 0;
    iCountDown = 0;
    iCount = 0;
    iTimeStamps.clear();
    formatManager.registerBasicFormats();
    transportSource.addChangeListener(this);
    recordThread.startThread();
    waveform.clear();
}

AutoSamplerAudioProcessor::~AutoSamplerAudioProcessor()
{
    stopRecording();
}

void AutoSamplerAudioProcessor::armRecording()
{
    dSampleRate = getSampleRate();
    iCountDown = dSampleRate * 4; // 4 seconds
    iCount = 4;
    recordState = RECORD_ARMED;
    runState = RUNNING;
}
void AutoSamplerAudioProcessor::startRecording()
{
    if (recordState == RECORD_ARMED)
    {
        dSampleRate = getSampleRate();
        
        if (dSampleRate)
        {
            juce::String file = sampleDirectory + "/" + sampleName[iSampleIndex] + ".wav";
            outputFile.operator=(file); // change file name
            outputFile.deleteFile();
            outputFile.create(); // overwrite existing file

            if (auto outputStream = std::unique_ptr<juce::FileOutputStream> (outputFile.createOutputStream()))
            {
                juce::WavAudioFormat wavFormat;
                
                if (auto writer = wavFormat.createWriterFor(outputStream.get(), dSampleRate, 2, 24, {}, 0))
                {
                    outputStream.release();
                    threadedWriter.reset(new juce::AudioFormatWriter::ThreadedWriter(writer, recordThread, 32768));
                    
                    const juce::ScopedLock sl (writerLock);
                    recordState = RECORDING;
                    activeWriter = threadedWriter.get();
                    iSample = 0;
                    iTimeStamps.clear();
                    iTimeStamps.push_back(0);
                    printf("RECORDING ACTIVE\n");
                }
            }
        }
    }
}

void AutoSamplerAudioProcessor::stopRecording()
{
    {
        const juce::ScopedLock sl (writerLock);
        activeWriter = nullptr;
    }
    
    recordState = RECORDING_OFF;
    runState = NOT_RUNNING;
    threadedWriter.reset();
    iTimeStamps.push_back(iSample);
}
//==============================================================================
const juce::String AutoSamplerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AutoSamplerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AutoSamplerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AutoSamplerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AutoSamplerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AutoSamplerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AutoSamplerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AutoSamplerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AutoSamplerAudioProcessor::getProgramName (int index)
{
    return {};
}

void AutoSamplerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AutoSamplerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    dSampleRate = sampleRate;
}

void AutoSamplerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AutoSamplerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void AutoSamplerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    dSampleRate = getSampleRate();
    iBufferSize = buffer.getNumSamples();
    
    if (runState == RUNNING && iCount > 0) {
        if (iCountDown > dSampleRate*3) // count down at ~1s intervals
            iCount = 4;
        else if (iCountDown > dSampleRate*2)
            iCount = 3;
        else if (iCountDown > dSampleRate*1)
            iCount = 2;
        else if (iCountDown > dSampleRate*0.5f)
            iCount = 1;
        else if (iCountDown > 0 && recordState == RECORD_ARMED)
            startRecording(); // start recording at ~500ms before 0
        else if (iCountDown <= 0)
            iCount = 0;
        iCountDown -= iBufferSize;
//        printf("%i\n", iCount);
    }
    
    const juce::ScopedLock sl (writerLock);
    
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

//    waveform.setNumChannels(totalNumInputChannels);
    waveform.setSamplesPerBlock(buffer.getNumSamples());
    waveform.setBufferSize(iBufferSize);
    waveform.pushBuffer(buffer);
    
    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
//    for (int channel = 0; channel < totalNumInputChannels; ++channel)
//    {
//        auto* channelData = buffer.getReadPointer (channel);

        // ..do something to the data...
        if (activeWriter.load() != nullptr)
        {
            if (recordState == RECORDING)
                activeWriter.load() -> write(buffer.getArrayOfReadPointers(), buffer.getNumSamples());
        }
//    }
}

//==============================================================================
bool AutoSamplerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AutoSamplerAudioProcessor::createEditor()
{
    return new AutoSamplerAudioProcessorEditor (*this);
}

//==============================================================================
void AutoSamplerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void AutoSamplerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AutoSamplerAudioProcessor();
}


//==============================================================================

