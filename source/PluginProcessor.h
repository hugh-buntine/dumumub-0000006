#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <map>
#include "Particle.h"

#if (MSVC)
#include "ipps.h"
#endif

class Canvas; // Forward declaration

// Simplified mass point data for physics simulation (processor side)
struct MassPointData
{
    juce::Point<float> position;
    float massMultiplier = 1.0f; // 1.0 for small, up to 4.0 for massive
};

// Simplified spawn point data for particle emission (processor side)
struct SpawnPointData
{
    juce::Point<float> position;
    float momentumAngle = 0.0f; // Direction particles should spawn (set by user via arrow)
    float visualRotation = 0.0f; // Current rotation angle for visual animation only
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
    
    // Audio file loading
    void loadAudioFile (const juce::File& file);
    juce::File getLoadedAudioFile() const { return loadedAudioFile; }
    bool hasAudioFileLoaded() const { return loadedAudioFile.existsAsFile(); }
    const juce::AudioBuffer<float>* getAudioBuffer() const { return &audioFileBuffer; }
    
    // Canvas reference for particle access
    void setCanvas (Canvas* canvasPtr) { canvas = canvasPtr; }
    
    // MIDI event injection (for UI-triggered notes)
    void injectMidiMessage (const juce::MidiMessage& message);
    
    // Parameter access
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    
    // Particle simulation access (thread-safe, for GUI drawing)
    juce::OwnedArray<Particle>* getParticles() { return &particles; }
    juce::CriticalSection& getParticlesLock() { return particlesLock; }
    
    // Mass and spawn point management (called from GUI thread)
    void updateMassPoint (int index, juce::Point<float> position, float massMultiplier);
    void addMassPoint (juce::Point<float> position, float massMultiplier);
    void removeMassPoint (int index);
    const std::vector<MassPointData>& getMassPoints() const { return massPoints; }
    
    void updateSpawnPoint (int index, juce::Point<float> position, float angle);
    void addSpawnPoint (juce::Point<float> position, float angle);
    void removeSpawnPoint (int index);
    const std::vector<SpawnPointData>& getSpawnPoints() const { return spawnPoints; }
    
    // Particle spawning (called from GUI or MIDI) - now requires MIDI note and ADSR parameters
    void spawnParticle (juce::Point<float> position, juce::Point<float> velocity,
                       float initialVelocity, float pitchShift, int midiNoteNumber,
                       float attackTime, float sustainLevel, float sustainLevelLinear, float releaseTime);
    
    // Gravity and canvas settings
    void setGravityStrength (float strength) { gravityStrength = strength; }
    void setCanvasBounds (juce::Rectangle<float> bounds) { canvasBounds = bounds; }
    void setParticleLifespan (float lifespan) { particleLifespan = lifespan; }
    void setMaxParticles (int max) { maxParticles = max; }
    void setBounceMode (bool enabled);
    bool getBounceMode() const { return bounceMode; }
    
    // AudioProcessorValueTreeState::Listener implementation
    void parameterChanged (const juce::String& parameterID, float newValue) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
    
    // Parameters
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Audio file storage
    juce::File loadedAudioFile;
    juce::AudioBuffer<float> audioFileBuffer;
    double audioFileSampleRate = 0.0;
    
    // Canvas reference (not owned) - kept for backward compatibility during transition
    Canvas* canvas = nullptr;
    
    // Pending MIDI messages from UI
    juce::MidiBuffer pendingMidiMessages;
    juce::CriticalSection midiLock;
    
    //==============================================================================
    // Particle simulation (runs in audio thread)
    juce::OwnedArray<Particle> particles;
    juce::CriticalSection particlesLock; // Protects particle array
    
    // MIDI note to particle mapping for ADSR control
    std::map<int, std::vector<int>> activeNoteToParticles; // Maps MIDI note number -> particle indices
    
    std::vector<MassPointData> massPoints;
    std::vector<SpawnPointData> spawnPoints;
    
    // Simulation parameters
    float gravityStrength = 100.0f;
    juce::Rectangle<float> canvasBounds {0, 0, 800, 600};
    float particleLifespan = 30.0f; // Legacy parameter - no longer used with ADSR
    int maxParticles = 8; // Default limit (can be changed via setMaxParticles)
    bool bounceMode = false; // When true, particles bounce off walls instead of wrapping
    
    // Timing for particle updates
    double lastUpdateTime = 0.0;
    
    // Buffer boundary continuity (to prevent clicks when grains overlap)
    float lastBufferOutputLeft = 0.0f;
    float lastBufferOutputRight = 0.0f;
    
    // MIDI note handling helpers
    void handleNoteOn (int noteNumber, float velocity, float pitchShift);
    void handleNoteOff (int noteNumber);
    
    // Helper method to update particle physics
    void updateParticleSimulation (double currentTime, int bufferSize);
};
