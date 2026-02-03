#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

//==============================================================================
enum class ADSRPhase
{
    Attack,
    Decay,
    Sustain,
    Release,
    Finished
};

//==============================================================================
struct Grain
{
    int startSample = 0;
    int playbackPosition = 0;
    int totalSamples = 0;
    bool active = true;
    int samplesRenderedThisBuffer = 0;
    
    Grain (int start, int size) : startSample (start), totalSamples (size) {}
};

//==============================================================================
class Particle
{
public:
    Particle (juce::Point<float> position, juce::Point<float> velocity, 
              const juce::Rectangle<float>& canvasBounds, int midiNoteNumber,
              float attackTime, float sustainLevel, float sustainLevelLinear, float releaseTime,
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
    ADSRPhase getADSRPhase() const { return adsrPhase; }
    
    // OPTIMIZATION: Unique ID for efficient note-to-particle mapping
    int getUniqueID() const { return uniqueID; }
    static int getNextUniqueID() { return nextUniqueID++; }
    
    // ADSR control
    void updateADSR (float deltaTime);
    void updateADSRSample (double sampleRate);
    void triggerRelease();
    float getADSRAmplitude() const { return adsrAmplitude; }
    float getADSRAmplitudeSmoothed() const { return adsrAmplitudeSmoothed; }
    
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
    
    float getPan() const;
    
    struct EdgeFade {
        float pan;       // -1.0 to 1.0
        float amplitude; // 1.0 in center, 0.0 at edges
    };
    EdgeFade getEdgeFade() const;
    
    float getGrainAmplitude (const Grain& grain) const;
    float getPitchShift() const { return pitchShift; }
    float getInitialVelocityMultiplier() const { return initialVelocityMultiplier; }
    
    // Advance grain playback and clean up finished grains
    void updateGrains (int numSamples);
    
    void setCanvasBounds (const juce::Rectangle<float>& bounds) { canvasBounds = bounds; }
    
    static void setStarImage (const juce::Image& image) { starImage = image; }
    
    static void initializeHannTable();
    
    void draw (juce::Graphics& g);

private:
    juce::Point<float> position;
    juce::Point<float> velocity;
    juce::Point<float> acceleration;
    float lifeTime = 0.0f;
    float radius = 3.0f;
    
    // OPTIMIZATION: Unique ID for efficient particle tracking
    int uniqueID;
    static int nextUniqueID;
    
    // ADSR envelope
    int midiNoteNumber = -1;
    ADSRPhase adsrPhase = ADSRPhase::Attack;
    float adsrTime = 0.0f;
    float attackTime = 0.01f;
    float sustainLevel = 0.7f;         // Logarithmic (for audio)
    float sustainLevelLinear = 0.7f;   // Linear (for visuals)
    float releaseTime = 0.5f;
    float adsrAmplitude = 0.0f;        // Logarithmic (for audio)
    float adsrAmplitudeLinear = 0.0f;  // Linear (for visuals)
    float adsrAmplitudeSmoothed = 0.0f;
    float releaseStartAmplitude = 0.0f;
    float releaseStartAmplitudeLinear = 0.0f;
    static constexpr float decayTime = 0.3f;
    
    // MIDI parameters
    float initialVelocityMultiplier = 1.0f;
    float pitchShift = 1.0f;
    
    // Trail system
    struct TrailPoint {
        juce::Point<float> position;
        float age = 0.0f;
    };
    std::vector<TrailPoint> trail;
    static constexpr int maxTrailPoints = 60;
    static constexpr float trailFadeTime = 1.0f;
    
    // Canvas bounds (order matters for constructor initializer list)
    juce::Rectangle<float> canvasBounds;
    
    bool bounceMode = false;
    
    // Active grains (can have multiple overlapping)
    std::vector<Grain> activeGrains;
    static constexpr int MAX_GRAINS_PER_PARTICLE = 8;
    
    // Grain parameters
    float grainSizeMs = 50.0f;
    
    // Grain triggering (order matters for constructor initializer list)
    double currentSampleRate = 44100.0;
    int samplesSinceLastGrainTrigger = 0;
    bool isFirstGrain = true;
    
    // Cached sample rate calculations
    int cachedTotalGrainSamples = 2205;
    int cachedAttackSamples = 220;
    int cachedReleaseSamples = 220;
    
    // Grain envelope lookup table (Hann window)
    static constexpr int envelopeLUTSize = 512;
    static std::array<float, envelopeLUTSize> sharedEnvelopeLUT;
    static bool envelopeLUTInitialized;
    static void initializeEnvelopeLUT();
    
    // Wraparound smoothing to prevent clicks
    bool justWrappedAround = false;
    float wraparoundSmoothingTime = 0.0f;
    static constexpr float wraparoundSmoothDuration = 0.05f;
    juce::Point<float> lastPosition;
    float lastGrainStartSample = 0.0f;
    
    static juce::Image starImage;
    
    // Hann window lookup table
    static std::vector<float> hannWindowTable;
    static constexpr int HANN_TABLE_SIZE = 512;
    static float getHannWindowValue (float normalizedPosition);
};