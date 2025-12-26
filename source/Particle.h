#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
// ADSR envelope phases for particle lifetime control
enum class ADSRPhase
{
    Attack,     // Fading in from 0 to 1
    Decay,      // Dropping from 1 to sustain level
    Sustain,    // Held at sustain level while MIDI key is held
    Release,    // MIDI key released, fading from sustain to 0
    Finished    // Release complete, particle should be removed
};

//==============================================================================
// Single grain instance
struct Grain
{
    int startSample = 0;        // Where in the audio buffer this grain starts
    int playbackPosition = 0;   // Current playback position within this grain
    int totalSamples = 0;       // Total size of THIS grain (captured at creation time)
    bool active = true;         // Is this grain still playing?
    int samplesRenderedThisBuffer = 0; // BUG #14 FIX: Track actual samples rendered per buffer
    
    Grain (int start, int size) : startSample (start), totalSamples (size) {}
};

//==============================================================================
class Particle
{
public:
    Particle (juce::Point<float> position, juce::Point<float> velocity, 
              const juce::Rectangle<float>& canvasBounds, int midiNoteNumber,
              float attackTime, float sustainLevel, float releaseTime,
              float initialVelocity = 1.0f, float pitchShift = 1.0f);
    ~Particle();

    //==============================================================================
    void update (float deltaTime);
    void applyForce (const juce::Point<float>& force);
    void wrapAround (const juce::Rectangle<float>& bounds);
    void bounceOff (const juce::Rectangle<float>& bounds);
    
    // Getters
    juce::Point<float> getPosition() const { return position; }
    juce::Point<float> getVelocity() const { return velocity; }
    float getLifeTime() const { return lifeTime; }
    bool isFinished() const { return adsrPhase == ADSRPhase::Finished; }
    int getMidiNoteNumber() const { return midiNoteNumber; }
    
    // ADSR control
    void updateADSR (float deltaTime);
    void triggerRelease();
    float getADSRAmplitude() const { return adsrAmplitude; }
    
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
    
    // Set bounce mode (affects edge panning behavior)
    void setBounceMode (bool enabled) { bounceMode = enabled; }
    bool getBounceMode() const { return bounceMode; }
    
    // Update grain start position based on current Y position (for continuous grains)
    int calculateGrainStartPosition (int bufferLength);
    
    // Trigger a new grain
    void triggerNewGrain (int bufferLength);
    
    // Calculate current pan and amplitude based on position
    float getPan() const; // -1.0 (left) to 1.0 (right)
    
    // Get edge fade information (simple amplitude fade at boundaries)
    struct EdgeFade {
        float pan;              // Pan position (-1.0 to 1.0)
        float amplitude;        // Fade multiplier: 1.0 in center, 0.0 at edges
    };
    EdgeFade getEdgeFade() const;
    
    float getGrainAmplitude (const Grain& grain) const; // Grain envelope with hardcoded 50% crossfade
    float getPitchShift() const { return pitchShift; } // Pitch shift multiplier for grain playback
    float getInitialVelocityMultiplier() const { return initialVelocityMultiplier; } // MIDI velocity (0.0 to 1.0)
    
    // Advance grain playback and clean up finished grains
    void updateGrains (int numSamples);
    
    // Set canvas bounds (needed for pan calculation)
    void setCanvasBounds (const juce::Rectangle<float>& bounds) { canvasBounds = bounds; }
    
    // Set the star image for all particles to use
    static void setStarImage (const juce::Image& image) { starImage = image; }
    
    // Static Hann window lookup table initialization (call once at startup)
    static void initializeHannTable();
    
    // Draw the particle
    void draw (juce::Graphics& g);

private:
    juce::Point<float> position;
    juce::Point<float> velocity;
    juce::Point<float> acceleration;
    float lifeTime = 0.0f;
    float radius = 3.0f;
    
    // ADSR envelope for particle lifetime
    int midiNoteNumber = -1;           // Which MIDI note spawned this particle
    ADSRPhase adsrPhase = ADSRPhase::Attack;
    float adsrTime = 0.0f;             // Time spent in current ADSR phase
    float attackTime = 0.01f;          // Attack duration in seconds
    float sustainLevel = 0.7f;         // Sustain level (0.0-1.0, controllable)
    float releaseTime = 0.5f;          // Release duration in seconds
    float adsrAmplitude = 0.0f;        // Current envelope amplitude (0.0-1.0)
    static constexpr float decayTime = 0.2f;  // Decay duration (fixed at 200ms)
    
    // MIDI parameters
    float initialVelocityMultiplier = 1.0f; // MIDI velocity mapped to 0.0-1.0
    float pitchShift = 1.0f; // Playback rate multiplier (1.0 = normal, 2.0 = octave up, 0.5 = octave down)
    
    // Trail system
    struct TrailPoint {
        juce::Point<float> position;
        float age = 0.0f; // Age of this trail point in seconds
    };
    std::vector<TrailPoint> trail;
    static constexpr int maxTrailPoints = 60; // ~1 second at 60 FPS
    static constexpr float trailFadeTime = 1.0f; // Trail fades over 1 second
    
    // Canvas bounds for position mapping
    juce::Rectangle<float> canvasBounds;
    
    // Bounce mode (affects edge panning behavior)
    bool bounceMode = false;
    
    // Active grains (can have multiple overlapping)
    std::vector<Grain> activeGrains;
    static constexpr int MAX_GRAINS_PER_PARTICLE = 8; // CPU optimization: limit concurrent grains
    
    // Grain audio parameters
    float grainSizeMs = 50.0f;      // Grain duration in milliseconds
    // Note: Grain attack/release are now HARDCODED to 50% crossfade (not parameters)
    // Particle-level attack/release control overall ADSR envelope instead
    
    // Grain triggering based on frequency
    int samplesSinceLastGrainTrigger = 0;
    bool isFirstGrain = true; // First grain triggers immediately
    
    // Cached sample rate calculations
    double currentSampleRate = 44100.0;
    int cachedTotalGrainSamples = 2205; // 50ms at 44.1kHz
    int cachedAttackSamples = 220; // 5ms at 44.1kHz
    int cachedReleaseSamples = 220; // 5ms at 44.1kHz
    
    // Wraparound smoothing to prevent clicks
    bool justWrappedAround = false;
    float wraparoundSmoothingTime = 0.0f;
    static constexpr float wraparoundSmoothDuration = 0.05f; // 50ms smooth transition (increased from 20ms)
    juce::Point<float> lastPosition;
    float lastGrainStartSample = 0.0f; // Track grain position for smooth interpolation
    
    // Static star image shared by all particles
    static juce::Image starImage;
    
    // Static Hann window lookup table (shared by all particles for efficiency)
    static std::vector<float> hannWindowTable;
    static constexpr int HANN_TABLE_SIZE = 512; // 512 samples provides smooth interpolation
    static float getHannWindowValue (float normalizedPosition); // Fast lookup with linear interpolation
};

