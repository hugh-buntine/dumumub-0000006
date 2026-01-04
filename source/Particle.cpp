#include "Particle.h"
#include "Logger.h"

// Initialize static members
juce::Image Particle::starImage;
std::vector<float> Particle::hannWindowTable;

// Initialize Hann window lookup table (call once at app startup)
void Particle::initializeHannTable()
{
    if (!hannWindowTable.empty())
        return; // Already initialized
    
    hannWindowTable.resize (HANN_TABLE_SIZE);
    
    const float pi = juce::MathConstants<float>::pi;
    for (int i = 0; i < HANN_TABLE_SIZE; ++i)
    {
        float normalizedPos = static_cast<float>(i) / static_cast<float>(HANN_TABLE_SIZE - 1);
        hannWindowTable[i] = 0.5f * (1.0f - std::cos (2.0f * pi * normalizedPos));
    }
}

// Fast Hann window lookup with linear interpolation
float Particle::getHannWindowValue (float normalizedPosition)
{
    if (hannWindowTable.empty())
        initializeHannTable(); // Lazy initialization fallback
    
    // Clamp position to valid range
    normalizedPosition = juce::jlimit (0.0f, 1.0f, normalizedPosition);
    
    // Convert to table index (floating point for interpolation)
    float tablePos = normalizedPosition * static_cast<float>(HANN_TABLE_SIZE - 1);
    int index0 = static_cast<int>(tablePos);
    int index1 = juce::jmin (index0 + 1, HANN_TABLE_SIZE - 1);
    
    // Linear interpolation between table entries
    float frac = tablePos - static_cast<float>(index0);
    float value = hannWindowTable[index0] + frac * (hannWindowTable[index1] - hannWindowTable[index0]);
    
    // Flush denormals to zero to prevent noise artifacts
    // When values get extremely small (< 1e-15), they can cause audible artifacts
    if (std::abs(value) < 1e-15f)
        value = 0.0f;
    
    return value;
}

//==============================================================================
Particle::Particle (juce::Point<float> position, juce::Point<float> velocity, 
                    const juce::Rectangle<float>& canvasBounds, int midiNoteNumber,
                    float attackTime, float sustainLevel, float sustainLevelLinear, float releaseTime,
                    float initialVelocity, float pitchShift)
    : position (position), velocity (velocity), canvasBounds (canvasBounds),
      lifeTime (0.0f), 
      midiNoteNumber (midiNoteNumber),
      adsrPhase (ADSRPhase::Attack),
      adsrTime (0.0f),
      attackTime (attackTime),
      sustainLevel (sustainLevel),              // Logarithmic value for audio
      sustainLevelLinear (sustainLevelLinear),  // Linear slider value for visuals
      releaseTime (releaseTime),
      adsrAmplitude (0.0f),                     // Logarithmic for audio
      adsrAmplitudeLinear (0.0f),               // Linear for visuals
      initialVelocityMultiplier (initialVelocity), pitchShift (pitchShift),
      currentSampleRate (0.0), samplesSinceLastGrainTrigger (0),
      cachedTotalGrainSamples (2205),
      lastPosition (position)
{
    // Reserve space for grains to avoid allocations during audio processing
    // With 100Hz grain frequency, we might have ~20 overlapping grains at most
    activeGrains.reserve (32);
}

void Particle::triggerNewGrain (int bufferLength)
{
    // CPU Optimization: Limit concurrent grains with voice stealing
    if (activeGrains.size() >= MAX_GRAINS_PER_PARTICLE)
    {
        // Find the grain with the lowest amplitude (furthest along in playback)
        // This is a simple voice stealing strategy - remove the quietest grain
        int oldestGrainIndex = 0;
        int maxPlaybackPos = 0;
        
        for (size_t i = 0; i < activeGrains.size(); ++i)
        {
            if (activeGrains[i].playbackPosition > maxPlaybackPos)
            {
                maxPlaybackPos = activeGrains[i].playbackPosition;
                oldestGrainIndex = static_cast<int>(i);
            }
        }
        
        // Remove the oldest grain
        activeGrains.erase (activeGrains.begin() + oldestGrainIndex);
        
        // Log voice stealing events (can cause clicks)
        static int voiceStealCount = 0;
        voiceStealCount++;
        if (voiceStealCount % 50 == 1)
        {
            LOG_WARNING("VOICE STEALING: Max grains reached (" + juce::String(MAX_GRAINS_PER_PARTICLE) + 
                       "), removed grain at pos " + juce::String(maxPlaybackPos) +
                       " (count: " + juce::String(voiceStealCount) + ")");
        }
    }
    
    // DIAGNOSTIC: Check for grain size mismatches in overlapping grains
    if (!activeGrains.empty())
    {
        static int sizeMismatchCheckCount = 0;
        sizeMismatchCheckCount++;
        
        if (sizeMismatchCheckCount % 100 == 0)
        {
            // Check if new grain size differs from existing grains
            bool sizeMismatch = false;
            int minSize = cachedTotalGrainSamples;
            int maxSize = cachedTotalGrainSamples;
            
            for (const auto& grain : activeGrains)
            {
                if (grain.totalSamples != cachedTotalGrainSamples)
                {
                    sizeMismatch = true;
                    minSize = juce::jmin(minSize, grain.totalSamples);
                    maxSize = juce::jmax(maxSize, grain.totalSamples);
                }
            }
            
            if (sizeMismatch)
            {
                LOG_WARNING("GRAIN SIZE MISMATCH: Overlapping grains have different sizes: " +
                           juce::String(minSize) + " to " + juce::String(maxSize) + 
                           " samples (fade regions may not align, causing clicks)");
            }
        }
    }
    
    int startSample = calculateGrainStartPosition (bufferLength);
    
    // Log sudden jumps in grain start position (can cause phase discontinuities/clicks)
    static int lastStartSample = startSample;
    static int positionJumpCount = 0;
    int jumpDistance = std::abs(startSample - lastStartSample);
    
    // Only log if jump is >5% of buffer and happens frequently (indicates Y position changing rapidly)
    if (jumpDistance > bufferLength / 20 && positionJumpCount % 100 == 0)
    {
        LOG_WARNING("GRAIN START POSITION JUMP: " + juce::String(lastStartSample) + " -> " + 
                   juce::String(startSample) + " (delta: " + juce::String(jumpDistance) + 
                   " samples, may cause phase issues)");
    }
    if (jumpDistance > bufferLength / 20)
        positionJumpCount++;
    
    lastStartSample = startSample;
    
    // Create grain with its own size snapshot (so changing grain size doesn't affect existing grains)
    activeGrains.push_back (Grain (startSample, cachedTotalGrainSamples));
    
    // Logging disabled in audio thread to prevent dropouts
    // LOG_INFO("Grain triggered at sample " + juce::String(startSample) + 
    //          ", total active grains: " + juce::String(activeGrains.size()));
}

void Particle::updateGrains (int numSamples)
{
    samplesSinceLastGrainTrigger += numSamples;
    
    // Track grain removals for click diagnosis
    static int totalGrainsRemoved = 0;
    int grainsBeforeUpdate = activeGrains.size();
    
    // BUG #14 FIX: Update each grain by its ACTUAL samples rendered, not buffer size
    // This prevents the 482-sample overshoot that causes grains to be removed mid-fade
    for (auto& grain : activeGrains)
    {
        // Use per-grain sample count if tracked, otherwise fall back to buffer size
        // (Fall back handles case where grain wasn't rendered this buffer, e.g. empty grains list)
        int advanceAmount = (grain.samplesRenderedThisBuffer > 0) 
                          ? grain.samplesRenderedThisBuffer 
                          : numSamples;
        
        grain.playbackPosition += advanceAmount;
        
        // Mark as inactive if finished
        if (grain.playbackPosition >= cachedTotalGrainSamples)
        {
            grain.active = false;
            
            // Log first few grain completions to see if ending causes clicks
            totalGrainsRemoved++;
            if (totalGrainsRemoved <= 5)
            {
                LOG_INFO("GRAIN COMPLETED #" + juce::String(totalGrainsRemoved) + 
                        ": startSample=" + juce::String(grain.startSample) +
                        ", finalPos=" + juce::String(grain.playbackPosition) + 
                        ", grainSize=" + juce::String(cachedTotalGrainSamples) +
                        ", advanced=" + juce::String(advanceAmount) +
                        " (overshoot: " + juce::String(grain.playbackPosition - cachedTotalGrainSamples) + ")");
            }
        }
    }
    
    // Remove finished grains
    activeGrains.erase (
        std::remove_if (activeGrains.begin(), activeGrains.end(),
                       [](const Grain& g) { return !g.active; }),
        activeGrains.end()
    );
}

Particle::~Particle()
{
}

//==============================================================================
void Particle::updateADSR (float deltaTime)
{
    adsrTime += deltaTime;
    
    switch (adsrPhase)
    {
        case ADSRPhase::Attack:
            // Exponential ramp from 0.0 to 1.0 over attackTime (feels more natural)
            if (attackTime > 0.0f)
            {
                float linearProgress = juce::jmin (1.0f, adsrTime / attackTime);
                // Apply exponential curve: y = x^2 (slow start, quick finish)
                adsrAmplitude = linearProgress * linearProgress;
                adsrAmplitudeLinear = adsrAmplitude; // Same for attack
            }
            else
            {
                adsrAmplitude = 1.0f; // Instant attack
                adsrAmplitudeLinear = 1.0f;
            }
            
            // Move to Decay phase when attack complete
            if (adsrAmplitude >= 1.0f)
            {
                adsrPhase = ADSRPhase::Decay;
                adsrTime = 0.0f;
            }
            break;
            
        case ADSRPhase::Decay:
            // Exponential decay from 1.0 to sustain level (quick drop, slow tail)
            if (decayTime > 0.0f)
            {
                float linearProgress = juce::jmin (1.0f, adsrTime / decayTime);
                // Apply inverse exponential: y = 1 - (1-x)^3 (quick drop initially)
                float curve = 1.0f - std::pow (1.0f - linearProgress, 3.0f);
                
                // Logarithmic amplitude for audio (uses converted sustainLevel)
                adsrAmplitude = 1.0f - (curve * (1.0f - sustainLevel));
                adsrAmplitude = juce::jmax (sustainLevel, adsrAmplitude);
                
                // Linear amplitude for visuals (uses linear slider value)
                adsrAmplitudeLinear = 1.0f - (curve * (1.0f - sustainLevelLinear));
                adsrAmplitudeLinear = juce::jmax (sustainLevelLinear, adsrAmplitudeLinear);
            }
            else
            {
                adsrAmplitude = sustainLevel;
                adsrAmplitudeLinear = sustainLevelLinear;
            }
            
            // Move to Sustain phase when decay complete
            if (adsrTime >= decayTime)
            {
                adsrPhase = ADSRPhase::Sustain;
                adsrAmplitude = sustainLevel;
                adsrAmplitudeLinear = sustainLevelLinear;
                adsrTime = 0.0f;
            }
            break;
            
        case ADSRPhase::Sustain:
            // Stay at sustain level while MIDI key is held
            adsrAmplitude = sustainLevel;
            adsrAmplitudeLinear = sustainLevelLinear;
            break;
            
        case ADSRPhase::Release:
            // Exponential release from sustain level to 0.0 (smooth fade)
            if (releaseTime > 0.0f)
            {
                float linearProgress = juce::jmin (1.0f, adsrTime / releaseTime);
                // Apply exponential decay: y = (1-x)^4 (smooth, natural fade)
                float curve = std::pow (1.0f - linearProgress, 4.0f);
                
                // Logarithmic for audio
                adsrAmplitude = sustainLevel * curve;
                adsrAmplitude = juce::jmax (0.0f, adsrAmplitude);
                
                // Linear for visuals
                adsrAmplitudeLinear = sustainLevelLinear * curve;
                adsrAmplitudeLinear = juce::jmax (0.0f, adsrAmplitudeLinear);
            }
            else
            {
                adsrAmplitude = 0.0f; // Instant release
                adsrAmplitudeLinear = 0.0f;
            }
            
            // Move to Finished phase when release complete
            if (adsrAmplitude <= 0.0f)
            {
                adsrPhase = ADSRPhase::Finished;
            }
            break;
            
        case ADSRPhase::Finished:
            adsrAmplitude = 0.0f;
            adsrAmplitudeLinear = 0.0f;
            break;
    }
}

void Particle::updateADSRSample (double sampleRate)
{
    // Update ADSR for a single sample (1/sampleRate seconds)
    // This provides smooth ADSR transitions even during short attack/release times
    if (sampleRate <= 0.0)
        return;
    
    float deltaTime = static_cast<float>(1.0 / sampleRate);
    updateADSR (deltaTime);
}

void Particle::triggerRelease()
{
    // Transition from Attack, Decay, or Sustain to Release phase
    if (adsrPhase == ADSRPhase::Attack || adsrPhase == ADSRPhase::Decay || adsrPhase == ADSRPhase::Sustain)
    {
        adsrPhase = ADSRPhase::Release;
        adsrTime = 0.0f;
    }
}

//==============================================================================
void Particle::update (float deltaTime)
{
    // Update ADSR envelope
    updateADSR (deltaTime);
    
    // Update wraparound smoothing timer
    if (justWrappedAround)
    {
        wraparoundSmoothingTime += deltaTime;
        if (wraparoundSmoothingTime >= wraparoundSmoothDuration)
        {
            justWrappedAround = false;
            wraparoundSmoothingTime = 0.0f;
        }
    }
    
    // Add current position to trail
    if (trail.empty() || position.getDistanceFrom(trail.back().position) > 2.0f)
    {
        TrailPoint newPoint;
        newPoint.position = position;
        newPoint.age = 0.0f;
        trail.push_back (newPoint);
        
        // Limit trail length
        if (trail.size() > maxTrailPoints)
            trail.erase (trail.begin());
    }
    
    // Age all trail points
    for (auto& point : trail)
        point.age += deltaTime;
    
    // Remove trail points that are too old
    trail.erase (
        std::remove_if (trail.begin(), trail.end(),
                       [](const TrailPoint& p) { return p.age > trailFadeTime; }),
        trail.end()
    );
    
    // Store last position before updating
    lastPosition = position;
    
    // Update velocity based on acceleration
    velocity += acceleration * deltaTime;
    
    // Update position based on velocity
    position += velocity * deltaTime;
    
    // Reset acceleration
    acceleration = juce::Point<float> (0.0f, 0.0f);
    
    // Update lifetime
    lifeTime += deltaTime;
}

void Particle::applyForce (const juce::Point<float>& force)
{
    acceleration += force;
}

void Particle::wrapAround (const juce::Rectangle<float>& bounds)
{
    // Simple wraparound - just teleport the position
    // The edge fade system will handle the smooth audio transition
    
    // Wrap horizontally
    if (position.x < bounds.getX())
        position.x = bounds.getRight();
    else if (position.x > bounds.getRight())
        position.x = bounds.getX();
    
    // Wrap vertically
    if (position.y < bounds.getY())
        position.y = bounds.getBottom();
    else if (position.y > bounds.getBottom())
        position.y = bounds.getY();
}

void Particle::bounceOff (const juce::Rectangle<float>& bounds)
{
    // Bounce off walls by reversing velocity and constraining position
    
    // Bounce horizontally
    if (position.x < bounds.getX())
    {
        position.x = bounds.getX();
        velocity.x = std::abs(velocity.x); // Make positive (moving right)
    }
    else if (position.x > bounds.getRight())
    {
        position.x = bounds.getRight();
        velocity.x = -std::abs(velocity.x); // Make negative (moving left)
    }
    
    // Bounce vertically
    if (position.y < bounds.getY())
    {
        position.y = bounds.getY();
        velocity.y = std::abs(velocity.y); // Make positive (moving down)
    }
    else if (position.y > bounds.getBottom())
    {
        position.y = bounds.getBottom();
        velocity.y = -std::abs(velocity.y); // Make negative (moving up)
    }
}

void Particle::draw (juce::Graphics& g)
{
    // Calculate base alpha from ADSR amplitude (using LINEAR slider value for visual feedback)
    // Visual opacity matches slider position (0.5 = 50% opacity), not converted logarithmic audio amplitude
    float lifetimeAlpha = adsrAmplitudeLinear;
    
    // Get edge crossfade info (for visual wraparound)
    const float edgeFadeZone = 50.0f;
    float distanceFromLeft = position.x - canvasBounds.getX();
    float distanceFromRight = canvasBounds.getRight() - position.x;
    
    float mainAlpha = 1.0f;
    float ghostAlpha = 0.0f;
    juce::Point<float> ghostPosition = position;
    bool shouldDrawGhost = false;
    
    // Only fade and show ghost particles in wrap mode, not in bounce mode
    if (!bounceMode)
    {
        // Near left edge - fade main particle, show ghost on right
        if (distanceFromLeft < edgeFadeZone && canvasBounds.getWidth() > 0)
        {
            mainAlpha = distanceFromLeft / edgeFadeZone;
            ghostAlpha = 1.0f - mainAlpha;
            ghostPosition.x = position.x + canvasBounds.getWidth();
            shouldDrawGhost = true;
        }
        // Near right edge - fade main particle, show ghost on left
        else if (distanceFromRight < edgeFadeZone && canvasBounds.getWidth() > 0)
        {
            mainAlpha = distanceFromRight / edgeFadeZone;
            ghostAlpha = 1.0f - mainAlpha;
            ghostPosition.x = position.x - canvasBounds.getWidth();
            shouldDrawGhost = true;
        }
    }
    
    // Trail color is always off-white (#FFFFF2 = RGB 255, 255, 242)
    juce::Colour trailColor (255, 255, 242);
    
    // Helper lambda to draw trail from a given position offset
    auto drawTrailFrom = [&](juce::Point<float> offset, float alpha)
    {
        if (trail.size() > 1)
        {
            for (size_t i = 0; i < trail.size() - 1; ++i)
            {
                const auto& p1 = trail[i];
                const auto& p2 = trail[i + 1];
                
                // Skip drawing if distance is too large (particle teleported)
                float dx = p2.position.x - p1.position.x;
                float dy = p2.position.y - p1.position.y;
                float distanceSquared = dx * dx + dy * dy;
                
                // If distance > 100 pixels, it's a teleport - skip drawing this segment
                if (distanceSquared > 100.0f * 100.0f)
                    continue;
                
                // Calculate alpha: base transparency from particle lifetime and offset alpha, 
                // then fade as trail point ages
                float trailFade = 1.0f - (p1.age / trailFadeTime);
                float finalAlpha = lifetimeAlpha * alpha * trailFade * 0.6f; // 0.6 makes trail dimmer than particle
                
                // Draw line segment with off-white color, offset by the given amount
                g.setColour (trailColor.withAlpha (finalAlpha));
                g.drawLine (p1.position.x + offset.x, p1.position.y + offset.y, 
                           p2.position.x + offset.x, p2.position.y + offset.y, 
                           2.0f); // Line thickness
            }
        }
    };
    
    // Helper lambda to draw particle star at a given position
    auto drawStarAt = [&](juce::Point<float> pos, float alpha)
    {
        float combinedAlpha = lifetimeAlpha * alpha;
        
        if (starImage.isValid())
        {
            // Draw 15x15 star image centered on position with combined fade
            float starSize = 15.0f;
            g.setOpacity (combinedAlpha);
            g.drawImage (starImage, 
                        juce::Rectangle<float> (pos.x - starSize / 2, 
                                               pos.y - starSize / 2, 
                                               starSize, starSize),
                        juce::RectanglePlacement::fillDestination);
        }
        else
        {
            // Fallback to drawing a circle if star image not loaded
            juce::Colour particleColor = activeGrains.empty()
                ? juce::Colours::blue   // Blue when silent
                : juce::Colours::red;   // Red while playing
            g.setColour (particleColor.withAlpha (combinedAlpha));
            g.fillEllipse (pos.x - radius, pos.y - radius, radius * 2, radius * 2);
        }
    };
    
    // Draw main particle and trail
    drawTrailFrom (juce::Point<float>(0, 0), mainAlpha);
    drawStarAt (position, mainAlpha);
    
    // Draw ghost particle and trail on opposite edge if crossfading
    if (shouldDrawGhost && ghostAlpha > 0.0f)
    {
        juce::Point<float> ghostOffset (ghostPosition.x - position.x, 0);
        drawTrailFrom (ghostOffset, ghostAlpha);
        drawStarAt (ghostPosition, ghostAlpha);
    }
}

//==============================================================================
// Audio grain methods

void Particle::updateSampleRate (double sampleRate)
{
    if (sampleRate != currentSampleRate && sampleRate > 0)
    {
        currentSampleRate = sampleRate;
        
        // Recalculate all sample-based values
        cachedTotalGrainSamples = static_cast<int>((grainSizeMs / 1000.0f) * sampleRate);
        
        // Hardcoded 50% crossfade: attack/release samples are each half the grain
        int halfGrainSamples = cachedTotalGrainSamples / 2;
        cachedAttackSamples = halfGrainSamples;  // 50% of grain
        cachedReleaseSamples = halfGrainSamples; // 50% of grain
        
        // Logging disabled in audio thread to prevent dropouts
        // LOG_INFO("Particle sample rate updated: " + juce::String(sampleRate) + " Hz, " +
        //          "grain samples: " + juce::String(cachedTotalGrainSamples));
    }
}

void Particle::setGrainParameters (float grainSizeMsNew, float attackPercentNew, float releasePercentNew)
{
    // Note: attack/release parameters are now ignored - grain crossfade is hardcoded to 50%
    // These parameters are kept for API compatibility but only grain size is used
    
    // Only update if grain size changed
    if (grainSizeMs != grainSizeMsNew)
    {
        grainSizeMs = grainSizeMsNew;
        
        // Recalculate cached grain size in samples
        cachedTotalGrainSamples = static_cast<int>((grainSizeMs / 1000.0f) * currentSampleRate);
    }
}

bool Particle::shouldTriggerNewGrain (double sampleRate, float grainFrequencyHz)
{
    // First grain triggers immediately
    if (isFirstGrain)
    {
        isFirstGrain = false;
        samplesSinceLastGrainTrigger = 0;
        return true;
    }
    
    // Calculate samples per grain period based on frequency
    int samplesPerPeriod = static_cast<int>(sampleRate / grainFrequencyHz);
    
    // Check if enough time has passed
    if (samplesSinceLastGrainTrigger >= samplesPerPeriod)
    {
        // DIAGNOSTIC: Check if triggering new grain while others are in their fade regions
        static int triggerCheckCount = 0;
        triggerCheckCount++;
        
        if (triggerCheckCount % 200 == 0 && !activeGrains.empty())
        {
            int fadeSamples = static_cast<int>(0.030f * sampleRate); // 30ms
            int grainsInFadeIn = 0;
            int grainsInFadeOut = 0;
            
            for (const auto& grain : activeGrains)
            {
                int actualFadeIn = juce::jmin(fadeSamples, grain.totalSamples / 2);
                int actualFadeOut = juce::jmin(fadeSamples, grain.totalSamples / 2);
                
                if (grain.playbackPosition < actualFadeIn)
                    grainsInFadeIn++;
                else if (grain.playbackPosition >= grain.totalSamples - actualFadeOut)
                    grainsInFadeOut++;
            }
            
            if (grainsInFadeIn > 0 || grainsInFadeOut > 0)
            {
                LOG_INFO("NEW GRAIN TRIGGER: " + juce::String(activeGrains.size()) + 
                        " existing grains (" + juce::String(grainsInFadeIn) + " fading in, " +
                        juce::String(grainsInFadeOut) + " fading out), period=" + 
                        juce::String(samplesPerPeriod) + " samples");
            }
        }
        
        samplesSinceLastGrainTrigger = 0;
        return true;
    }
    
    return false;
}

float Particle::getPan() const
{
    // Map X position to pan: left edge = -1.0, right edge = 1.0
    if (canvasBounds.getWidth() <= 0)
        return 0.0f;
    
    float normalizedX = (position.x - canvasBounds.getX()) / canvasBounds.getWidth();
    return juce::jlimit (-1.0f, 1.0f, (normalizedX * 2.0f) - 1.0f);
}

Particle::EdgeFade Particle::getEdgeFade() const
{
    EdgeFade result;
    result.amplitude = 1.0f; // Always full amplitude, no fade out
    
    if (canvasBounds.getWidth() <= 0)
    {
        result.pan = 0.0f;
        return result;
    }
    
    const float edgeFadeZone = 50.0f; // 50px from edge starts pan transition
    float normalizedX = (position.x - canvasBounds.getX()) / canvasBounds.getWidth();
    
    // Calculate base pan: left edge = -1.0, center = 0.0, right edge = 1.0
    float basePan = juce::jlimit (-1.0f, 1.0f, (normalizedX * 2.0f) - 1.0f);
    
    // In bounce mode, allow full left/right panning without center transition
    if (bounceMode)
    {
        result.pan = basePan;
        return result;
    }
    
    // In wrap mode, transition to center at edges for seamless wraparound
    // Check if near left edge (x < edgeFadeZone)
    float distanceFromLeft = position.x - canvasBounds.getX();
    if (distanceFromLeft < edgeFadeZone)
    {
        // Transition pan from base position to center (0.0) as we approach left edge
        float edgeFactor = distanceFromLeft / edgeFadeZone; // 0.0 at edge, 1.0 at fadeZone boundary
        result.pan = basePan * edgeFactor; // Gradually moves towards 0.0 (center)
        return result;
    }
    
    // Check if near right edge (x > width - edgeFadeZone)
    float distanceFromRight = canvasBounds.getRight() - position.x;
    if (distanceFromRight < edgeFadeZone)
    {
        // Transition pan from base position to center (0.0) as we approach right edge
        float edgeFactor = distanceFromRight / edgeFadeZone; // 0.0 at edge, 1.0 at fadeZone boundary
        result.pan = basePan * edgeFactor; // Gradually moves towards 0.0 (center)
        return result;
    }
    
    // Not near any edge - use full pan based on position
    result.pan = basePan;
    return result;
}

float Particle::getGrainAmplitude (const Grain& grain) const
{
    // Use Hann window for smooth grain envelope (prevents clicks)
    // This creates a smooth bell curve from 0.0 → 1.0 → 0.0
    
    // Safety check: ensure grain size is valid
    if (grain.totalSamples <= 0)
    {
        // Only log this critical error once per particle
        static bool hasLogged = false;
        if (!hasLogged)
        {
            LOG_ERROR("getGrainAmplitude: Invalid grain size (grain.totalSamples = " + 
                     juce::String(grain.totalSamples) + ")");
            hasLogged = true;
        }
        return 0.0f;
    }
    
    int grainPos = grain.playbackPosition;
    
    // **CRITICAL FIX: Handle grain position beyond grain size**
    // When processing buffers, position can jump past the grain end (e.g., 10752 + 512 = 11264 > 11201)
    // In this case, treat it as fully faded out to prevent abrupt cutoff
    if (grainPos >= cachedTotalGrainSamples)
    {
        return 0.0f; // Grain has finished, fully faded out
    }
    
    // Log grain envelope values at START and END to verify proper fade-in/fade-out
    static int grainCallCount = 0;
    static int lastLoggedGrainStart = -1;
    bool isNewGrain = (grain.startSample != lastLoggedGrainStart);
    
    if (isNewGrain)
    {
        grainCallCount++;
        lastLoggedGrainStart = grain.startSample;
    }
    
    // **FIXED-DURATION FADE-IN/FADE-OUT to prevent clicks**
    // Problem: Applying Hann window to entire grain creates extremely slow fade-in for long grains
    // For 254ms grain (11,201 samples), Hann values at start are ~0.0000002, causing clicks
    // Solution: Use fixed 30ms fade-in/fade-out regardless of grain size
    // Increased from 10ms to 30ms to provide gentler attack and better handle non-zero-crossing starts
    
    const int fadeSamples = static_cast<int>(0.030f * currentSampleRate); // 30ms = ~1323 samples @ 44.1kHz
    
    // CRITICAL: Use the grain's OWN size, not the current particle grain size parameter
    // This allows existing grains to complete with their original size even if parameter changes
    const int grainTotalSamples = grain.totalSamples;
    const int halfGrain = grainTotalSamples / 2;
    
    // CRITICAL FIX: For short grains, use half the grain size for each fade to prevent overlap
    // If grain is shorter than 2× fade duration, fade-in and fade-out would overlap!
    const int actualFadeInSamples = juce::jmin(fadeSamples, halfGrain);
    const int actualFadeOutSamples = juce::jmin(fadeSamples, halfGrain);
    
    // Logging enabled to debug fade distances
    bool shouldLog = (grainCallCount <= 3) && (grainPos == 0 || grainPos == actualFadeInSamples || 
                                                grainPos == (grainTotalSamples - actualFadeOutSamples) ||
                                                grainPos == (grainTotalSamples - 1));
    
    if (shouldLog)
    {
        LOG_INFO("GRAIN #" + juce::String(grainCallCount) + 
                " pos=" + juce::String(grainPos) + "/" + juce::String(grainTotalSamples) +
                " fadeIn=" + juce::String(actualFadeInSamples) + 
                " fadeOut=" + juce::String(actualFadeOutSamples) +
                " baseFade=" + juce::String(fadeSamples));
    }
    
    float grainEnvelope = 1.0f; // Default to full volume in middle of grain
    
    if (grainPos < actualFadeInSamples)
    {
        // FADE IN: Use FIRST HALF of Hann curve (rising: 0.0 → 1.0)
        // Map grain position 0→actualFadeInSamples to Hann input 0.0→0.5
        // CRITICAL FIX: Use (actualFadeInSamples - 1) as denominator to reach exactly 1.0
        // When grainPos = 0 → fadeProgress = 0.0 (Hann = 0.0, silent start)
        // When grainPos = actualFadeInSamples - 1 → fadeProgress = 1.0 (Hann = 1.0, full volume)
        float fadeProgress = static_cast<float>(grainPos) / static_cast<float>(actualFadeInSamples - 1);
        fadeProgress = juce::jlimit (0.0f, 1.0f, fadeProgress);
        
        // Scale to first half of Hann curve: input 0.0 gives Hann(0.0)=0.0, input 1.0 gives Hann(0.5)=1.0
        float hannPos = fadeProgress * 0.5f; // Maps 0.0→1.0 to 0.0→0.5
        grainEnvelope = getHannWindowValue(hannPos);
        
        // Log critical fade-in points
        static int fadeInLogCount = 0;
        if (fadeInLogCount < 10 && (grainPos == 0 || grainPos == actualFadeInSamples - 1))
        {
            LOG_INFO("FADE-IN at pos=" + juce::String(grainPos) + 
                    ", envelope=" + juce::String(grainEnvelope, 6) +
                    ", fadeProgress=" + juce::String(fadeProgress, 4));
            fadeInLogCount++;
        }
    }
    else if (grainPos >= grainTotalSamples - actualFadeOutSamples)
    {
        // FADE OUT: Use SECOND HALF of Hann curve (falling: 1.0 → 0.0)
        // Map grain position to Hann input 0.5→1.0
        int samplesToEnd = grainTotalSamples - grainPos;
        
        // CRITICAL FIX: Map samplesToEnd correctly to reach exactly 1.0 at final sample
        // When samplesToEnd = actualFadeOutSamples → fadeProgress = 0.0 (fade just starting)
        // When samplesToEnd = 1 → fadeProgress = 1.0 (fade complete, must be zero!)
        // Need to use (actualFadeOutSamples - 1) as denominator for correct mapping
        float fadeProgress = 1.0f - (static_cast<float>(samplesToEnd - 1) / static_cast<float>(actualFadeOutSamples - 1));
        fadeProgress = juce::jlimit (0.0f, 1.0f, fadeProgress);
        
        // Scale to second half of Hann curve: 0.0 gives Hann(0.5)=1.0, 1.0 gives Hann(1.0)=0.0
        float hannPos = 0.5f + (fadeProgress * 0.5f); // Maps 0.0→1.0 to 0.5→1.0
        grainEnvelope = getHannWindowValue(hannPos);
        
        // Log critical fade-out points
        static int fadeOutLogCount = 0;
        if (fadeOutLogCount < 10 && (samplesToEnd <= 1 || samplesToEnd == actualFadeOutSamples))
        {
            LOG_INFO("FADE-OUT at pos=" + juce::String(grainPos) + 
                    ", samplesToEnd=" + juce::String(samplesToEnd) +
                    ", envelope=" + juce::String(grainEnvelope, 6) +
                    ", fadeProgress=" + juce::String(fadeProgress, 4));
            fadeOutLogCount++;
        }
    }
    // else: middle section stays at 1.0 (full volume)
    
    // Validate result - check for NaN or infinity
    if (!std::isfinite(grainEnvelope))
    {
        static int invalidEnvelopeCount = 0;
        invalidEnvelopeCount++;
        if (invalidEnvelopeCount % 500 == 1)
        {
            LOG_ERROR("INVALID ENVELOPE VALUE (NaN/Inf): grainPos = " + 
                     juce::String(grainPos) + " / " + juce::String(cachedTotalGrainSamples) +
                     " (occurrence #" + juce::String(invalidEnvelopeCount) + ")");
        }
        grainEnvelope = 0.0f;
    }
    
    // CRITICAL FIX: Return ONLY the grain envelope (Hann window)
    // DO NOT apply ADSR here - it causes amplitude modulation artifacts during Attack/Release
    // The ADSR envelope is applied at the particle level (to all grains combined) in PluginProcessor
    return grainEnvelope;
}

int Particle::calculateGrainStartPosition (int bufferLength)
{
    if (bufferLength <= 0 || canvasBounds.getHeight() <= 0)
        return 0;
    
    // Calculate grain start based on current Y position
    // Y=0 (top) = end of sample (1.0), Y=height (bottom) = start of sample (0.0)
    float normalizedY = 1.0f - (position.y / canvasBounds.getHeight());
    normalizedY = juce::jlimit (0.0f, 1.0f, normalizedY);
    
    int startSample = static_cast<int>(normalizedY * bufferLength);
    return juce::jlimit (0, bufferLength - 1, startSample);
}
