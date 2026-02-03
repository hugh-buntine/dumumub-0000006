#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <map>
#include "Particle.h"

#if (MSVC)
#include "ipps.h"
#endif

class Canvas;

struct MassPointData
{
    juce::Point<float> position;
    float massMultiplier = 1.0f;
};

struct SpawnPointData
{
    juce::Point<float> position;
    float momentumAngle = 0.0f;
    float visualRotation = 0.0f;
};

class PluginProcessor : public juce::AudioProcessor,
                         public juce::AudioProcessorValueTreeState::Listener
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
    
    void loadAudioFile (const juce::File& file);
    juce::File getLoadedAudioFile() const { return loadedAudioFile; }
    bool hasAudioFileLoaded() const { return loadedAudioFile.existsAsFile(); }
    const juce::AudioBuffer<float>* getAudioBuffer() const { return &audioFileBuffer; }
    
    void setCanvas (Canvas* canvasPtr) { canvas = canvasPtr; }
    void injectMidiMessage (const juce::MidiMessage& message);
    
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    
    juce::OwnedArray<Particle>* getParticles() { return &particles; }
    juce::CriticalSection& getParticlesLock() { return particlesLock; }
    
    void loadPointsFromTree();
    void savePointsToTree();
    
    void updateMassPoint (int index, juce::Point<float> position, float massMultiplier);
    void addMassPoint (juce::Point<float> position, float massMultiplier);
    void removeMassPoint (int index);
    const std::vector<MassPointData>& getMassPoints() const { return massPoints; }
    
    void updateSpawnPoint (int index, juce::Point<float> position, float angle);
    void addSpawnPoint (juce::Point<float> position, float angle);
    void removeSpawnPoint (int index);
    const std::vector<SpawnPointData>& getSpawnPoints() const { return spawnPoints; }
    
    void spawnParticle (juce::Point<float> position, juce::Point<float> velocity,
                       float initialVelocity, float pitchShift, int midiNoteNumber,
                       float attackTime, float sustainLevel, float sustainLevelLinear, float releaseTime);
    
    void setGravityStrength (float strength) { gravityStrength = strength; }
    void setCanvasBounds (juce::Rectangle<float> bounds) { canvasBounds = bounds; }
    void setParticleLifespan (float lifespan) { particleLifespan = lifespan; }
    void setMaxParticles (int max) { maxParticles = max; }
    void setBounceMode (bool enabled);
    bool getBounceMode() const { return bounceMode; }
    
    void parameterChanged (const juce::String& parameterID, float newValue) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
    
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    juce::File loadedAudioFile;
    juce::AudioBuffer<float> audioFileBuffer;
    double audioFileSampleRate = 0.0;
    
    Canvas* canvas = nullptr;
    
    juce::MidiBuffer pendingMidiMessages;
    juce::CriticalSection midiLock;
    
    juce::OwnedArray<Particle> particles;
    juce::CriticalSection particlesLock;
    
    // Maps MIDI note -> particle indices for ADSR release
    std::map<int, std::vector<int>> activeNoteToParticles;
    
    std::vector<MassPointData> massPoints;
    std::vector<SpawnPointData> spawnPoints;
    bool stateHasBeenRestored = false;
    
    float gravityStrength = 50000.0f;
    juce::Rectangle<float> canvasBounds {0, 0, 400, 400};
    float particleLifespan = 30.0f;
    int maxParticles = 8;
    bool bounceMode = false;
    
    double lastUpdateTime = 0.0;
    
    // Prevents clicks at buffer boundaries
    float lastBufferOutputLeft = 0.0f;
    float lastBufferOutputRight = 0.0f;
    
    void handleNoteOn (int noteNumber, float velocity, float pitchShift);
    void handleNoteOff (int noteNumber);
    void updateParticleSimulation (double currentTime, int bufferSize);
};
