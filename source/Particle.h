#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
// Single grain instance
struct Grain
{
    int startSample = 0;        // Where in the audio buffer this grain starts
    int playbackPosition = 0;   // Current playback position within this grain
    bool active = true;         // Is this grain still playing?
    
    Grain (int start) : startSample (start) {}
};

//==============================================================================
class Particle
{
public:
    Particle (juce::Point<float> position, juce::Point<float> velocity, 
              const juce::Rectangle<float>& canvasBounds, float maxLifeTime = 30.0f);
    ~Particle();

    //==============================================================================
    void update (float deltaTime);
    void applyForce (const juce::Point<float>& force);
    void wrapAround (const juce::Rectangle<float>& bounds);
    
    // Getters
    juce::Point<float> getPosition() const { return position; }
    juce::Point<float> getVelocity() const { return velocity; }
    float getLifeTime() const { return lifeTime; }
    bool isDead() const { return lifeTime > maxLifeTime; }
    
    // Audio grain getters
    float getGrainSizeMs() const { return grainSizeMs; }
    int getTotalGrainSamples() const { return cachedTotalGrainSamples; }
    
    // Access to active grains
    const std::vector<Grain>& getActiveGrains() const { return activeGrains; }
    std::vector<Grain>& getActiveGrains() { return activeGrains; }
    
    // Update total grain samples based on sample rate
    void updateSampleRate (double sampleRate);
    
    // Set grain parameters (attack and release are percentages 0-100)
    void setGrainParameters (float grainSizeMs, float attackPercent, float releasePercent);
    
    // Check if it's time to trigger a new grain based on frequency
    bool shouldTriggerNewGrain (double sampleRate, float grainFrequencyHz);
    
    // Setter to convert normalized position to actual sample index
    void setGrainStartSampleFromBuffer (int bufferLength);
    
    // Update grain start position based on current Y position (for continuous grains)
    int calculateGrainStartPosition (int bufferLength);
    
    // Trigger a new grain
    void triggerNewGrain (int bufferLength);
    
    // Calculate current pan and amplitude based on position
    float getPan() const; // -1.0 (left) to 1.0 (right)
    
    // Get edge crossfade information (for smooth wraparound)
    struct EdgeCrossfade {
        float mainPan;           // Primary pan position
        float crossfadePan;      // Secondary pan position (on opposite side)
        float crossfadeAmount;   // 0.0 = no crossfade, 1.0 = 50/50 mix
    };
    EdgeCrossfade getEdgeCrossfade() const;
    
    float getGrainAmplitude (const Grain& grain) const; // 0.0 to 1.0 based on envelope
    float getLifetimeAmplitude() const; // 0.0 to 1.0 based on age (fades as particle dies)
    
    // Advance grain playback and clean up finished grains
    void updateGrains (int numSamples);
    
    // Set canvas bounds (needed for pan calculation)
    void setCanvasBounds (const juce::Rectangle<float>& bounds) { canvasBounds = bounds; }
    
    // Draw the particle
    void draw (juce::Graphics& g);

private:
    juce::Point<float> position;
    juce::Point<float> velocity;
    juce::Point<float> acceleration;
    float lifeTime = 0.0f;
    float maxLifeTime = 30.0f; // 30 seconds before particle dies
    float radius = 3.0f;
    
    // Canvas bounds for position mapping
    juce::Rectangle<float> canvasBounds;
    
    // Active grains (can have multiple overlapping)
    std::vector<Grain> activeGrains;
    
    // Grain audio parameters
    float grainSizeMs = 50.0f;      // Grain duration in milliseconds
    float attackPercent = 20.0f;    // Attack envelope as % of first half
    float releasePercent = 20.0f;   // Release envelope as % of second half
    
    // Grain triggering based on frequency
    int samplesSinceLastGrainTrigger = 0;
    bool isFirstGrain = true; // First grain triggers immediately
    
    // Cached sample rate calculations
    double currentSampleRate = 44100.0;
    int cachedTotalGrainSamples = 2205; // 50ms at 44.1kHz
    int cachedAttackSamples = 220; // 5ms at 44.1kHz
    int cachedReleaseSamples = 220; // 5ms at 44.1kHz
};
