#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "SpawnPoint.h"
#include "MassPoint.h"
#include "Particle.h"

//==============================================================================
class Canvas : public juce::Component, 
               private juce::Timer,
               public juce::FileDragAndDropTarget
{
public:
    Canvas();
    ~Canvas() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void newSpawnPoint();
    void newMassPoint();
    void spawnParticle();
    void spawnParticleFromMidi (int midiNote, float midiVelocity); // Spawn with MIDI parameters
    void drawGravityWaves (juce::Graphics& g);
    void drawMomentumArrows (juce::Graphics& g);
    void drawParticles (juce::Graphics& g);
    void drawWaveform (juce::Graphics& g);
    
    // Timer callback for physics updates
    void timerCallback() override;
    
    // Mouse handling for arrow dragging
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;
    
    // File drag and drop
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;
    
    // Callback for when audio file is loaded
    std::function<void(const juce::File&)> onAudioFileLoaded;
    
    // Audio buffer reference
    void setAudioBuffer (const juce::AudioBuffer<float>* buffer);
    
    // Set particle lifespan for newly spawned particles
    void setParticleLifespan (float lifespanSeconds) { particleLifespan = lifespanSeconds; }
    
    // Access to particles for audio processing (thread-safe copy)
    juce::OwnedArray<Particle>* getParticles() { return &particles; }
    juce::CriticalSection& getParticlesLock() { return particlesLock; }

private:

    int maxSpawnPoints = 8;
    int maxMassPoints = 4;
    juce::OwnedArray<SpawnPoint> spawnPoints;
    juce::OwnedArray<MassPoint> massPoints;
    juce::OwnedArray<Particle> particles;
    
    bool showGravityWaves = true;
    int nextSpawnPointIndex = 0; // Round-robin particle emission
    float gravityStrength = 50000.0f; // Gravity force multiplier
    float particleLifespan = 30.0f; // Default lifespan for new particles (in seconds)
    
    // File drag state
    bool isDraggingFile = false;
    
    // Audio waveform data
    const juce::AudioBuffer<float>* audioBuffer = nullptr;
    
    // Arrow dragging state
    SpawnPoint* draggedArrowSpawnPoint = nullptr;
    static constexpr float minArrowLength = 20.0f;
    static constexpr float maxArrowLength = 50.0f;
    
    // Helper functions
    bool isMouseNearArrowTip (SpawnPoint* spawn, const juce::Point<float>& mousePos) const;
    juce::Point<float> getSpawnPointCenter (SpawnPoint* spawn) const;
    
    // Thread safety for particle access
    juce::CriticalSection particlesLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Canvas)
};
