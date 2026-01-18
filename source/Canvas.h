#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "SpawnPoint.h"
#include "MassPoint.h"
#include "Particle.h"
#include "CustomPopupMenuLookAndFeel.h"

// Forward declaration to avoid circular dependency
class PluginProcessor;

//==============================================================================
class Canvas : public juce::Component, 
               private juce::Timer,
               public juce::FileDragAndDropTarget
{
public:
    Canvas (PluginProcessor& processor);
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
    void drawSpawnPoints (juce::Graphics& g);
    void drawMassPoints (juce::Graphics& g);
    
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
    
    // Set bounce mode (particles bounce off walls instead of wrapping)
    void setBounceMode (bool enabled);
    
    // Set custom typeface for drop text display
    void setCustomTypeface (juce::Typeface::Ptr typeface) { customTypeface = typeface; }
    
    // Access to particles - now deprecated, particles live in processor
    // These are kept for backward compatibility during transition
    juce::OwnedArray<Particle>* getParticles();
    juce::CriticalSection& getParticlesLock();

private:
    
    // Reference to audio processor (owns the particle simulation)
    PluginProcessor& audioProcessor;

    bool bounceMode = false;
    int maxSpawnPoints = 8;
    int maxMassPoints = 4;
    int maxParticles = 8;
    juce::OwnedArray<SpawnPoint> spawnPoints;
    juce::OwnedArray<MassPoint> massPoints;
    // Particles now live in audioProcessor, not here!
    
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
    
    // Sync GUI components with processor data (for loading saved state)
    void syncGuiFromProcessor();
    
    // Custom popup menu look and feel
    CustomPopupMenuLookAndFeel popupMenuLookAndFeel;
    
    // Custom typeface for drop text display
    juce::Typeface::Ptr customTypeface;
    
    // Thread safety removed - particles lock now lives in processor

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Canvas)
};
