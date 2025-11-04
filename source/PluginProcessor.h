#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#if (MSVC)
#include "ipps.h"
#endif

class Canvas; // Forward declaration

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    // Audio file loading
    void loadAudioFile (const juce::File& file);
    juce::File getLoadedAudioFile() const { return loadedAudioFile; }
    bool hasAudioFileLoaded() const { return loadedAudioFile.existsAsFile(); }
    const juce::AudioBuffer<float>* getAudioBuffer() const { return &audioFileBuffer; }
    
    // Canvas reference for particle access
    void setCanvas (Canvas* canvasPtr) { canvas = canvasPtr; }
    
    // Parameter access
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
    
    // Parameters
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Audio file storage
    juce::File loadedAudioFile;
    juce::AudioBuffer<float> audioFileBuffer;
    double audioFileSampleRate = 0.0;
    
    // Canvas reference (not owned)
    Canvas* canvas = nullptr;
};
