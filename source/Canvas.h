#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "SpawnPoint.h"
#include "MassPoint.h"
#include "Particle.h"
#include "CustomPopupMenuLookAndFeel.h"

class PluginProcessor;

//==============================================================================
class Canvas : public juce::Component, 
               private juce::Timer,
               public juce::FileDragAndDropTarget
{
public:
    Canvas (PluginProcessor& processor);
    ~Canvas() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void newSpawnPoint();
    void newMassPoint();
    void spawnParticle();
    void spawnParticleFromMidi (int midiNote, float midiVelocity);
    void drawGravityWaves (juce::Graphics& g);
    void drawMomentumArrows (juce::Graphics& g);
    void drawParticles (juce::Graphics& g);
    void drawWaveform (juce::Graphics& g);
    void drawSpawnPoints (juce::Graphics& g);
    void drawMassPoints (juce::Graphics& g);
    
    void timerCallback() override;
    
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;
    
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;
    
    std::function<void(const juce::File&)> onAudioFileLoaded;
    
    void setAudioBuffer (const juce::AudioBuffer<float>* buffer);
    void setParticleLifespan (float lifespanSeconds) { particleLifespan = lifespanSeconds; }
    void setBounceMode (bool enabled);
    void setCustomTypeface (juce::Typeface::Ptr typeface) { customTypeface = typeface; }
    
    // Deprecated - particles live in processor. Kept for backward compatibility.
    juce::OwnedArray<Particle>* getParticles();
    juce::CriticalSection& getParticlesLock();

private:
    PluginProcessor& audioProcessor;

    bool bounceMode = false;
    int maxSpawnPoints = 8;
    int maxMassPoints = 4;
    int maxParticles = 8;
    juce::OwnedArray<SpawnPoint> spawnPoints;
    juce::OwnedArray<MassPoint> massPoints;
    
    bool showGravityWaves = true;
    int nextSpawnPointIndex = 0;
    float gravityStrength = 50000.0f;
    float particleLifespan = 30.0f;
    
    bool isDraggingFile = false;
    const juce::AudioBuffer<float>* audioBuffer = nullptr;
    
    SpawnPoint* draggedArrowSpawnPoint = nullptr;
    static constexpr float minArrowLength = 20.0f;
    static constexpr float maxArrowLength = 50.0f;
    
    bool isMouseNearArrowTip (SpawnPoint* spawn, const juce::Point<float>& mousePos) const;
    juce::Point<float> getSpawnPointCenter (SpawnPoint* spawn) const;
    void syncGuiFromProcessor();
    
    CustomPopupMenuLookAndFeel popupMenuLookAndFeel;
    juce::Typeface::Ptr customTypeface;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Canvas)
};
