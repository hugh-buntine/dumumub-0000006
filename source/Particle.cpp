#include "Particle.h"
#include "Logger.h"

// Initialize static member
juce::Image Particle::starImage;

//==============================================================================
Particle::Particle (juce::Point<float> position, juce::Point<float> velocity, 
                    const juce::Rectangle<float>& canvasBounds, int midiNoteNumber,
                    float attackTime, float sustainLevel, float releaseTime,
                    float initialVelocity, float pitchShift)
    : position (position), velocity (velocity), canvasBounds (canvasBounds),
      lifeTime (0.0f), 
      midiNoteNumber (midiNoteNumber),
      adsrPhase (ADSRPhase::Attack),
      adsrTime (0.0f),
      attackTime (attackTime),
      sustainLevel (sustainLevel),
      releaseTime (releaseTime),
      adsrAmplitude (0.0f),
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
    }
    
    int startSample = calculateGrainStartPosition (bufferLength);
    activeGrains.push_back (Grain (startSample));
    
    // Logging disabled in audio thread to prevent dropouts
    // LOG_INFO("Grain triggered at sample " + juce::String(startSample) + 
    //          ", total active grains: " + juce::String(activeGrains.size()));
}

void Particle::updateGrains (int numSamples)
{
    samplesSinceLastGrainTrigger += numSamples;
    
    // Update all active grains
    for (auto& grain : activeGrains)
    {
        grain.playbackPosition += numSamples;
        
        // Mark as inactive if finished
        if (grain.playbackPosition >= cachedTotalGrainSamples)
            grain.active = false;
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
            }
            else
                adsrAmplitude = 1.0f; // Instant attack
            
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
                adsrAmplitude = 1.0f - (curve * (1.0f - sustainLevel));
                adsrAmplitude = juce::jmax (sustainLevel, adsrAmplitude);
            }
            else
                adsrAmplitude = sustainLevel;
            
            // Move to Sustain phase when decay complete
            if (adsrTime >= decayTime)
            {
                adsrPhase = ADSRPhase::Sustain;
                adsrAmplitude = sustainLevel;
                adsrTime = 0.0f;
            }
            break;
            
        case ADSRPhase::Sustain:
            // Stay at sustain level while MIDI key is held
            adsrAmplitude = sustainLevel;
            break;
            
        case ADSRPhase::Release:
            // Exponential release from sustain level to 0.0 (smooth fade)
            if (releaseTime > 0.0f)
            {
                float linearProgress = juce::jmin (1.0f, adsrTime / releaseTime);
                // Apply exponential decay: y = (1-x)^4 (smooth, natural fade)
                float curve = std::pow (1.0f - linearProgress, 4.0f);
                adsrAmplitude = sustainLevel * curve;
                adsrAmplitude = juce::jmax (0.0f, adsrAmplitude);
            }
            else
                adsrAmplitude = 0.0f; // Instant release
            
            // Move to Finished phase when release complete
            if (adsrAmplitude <= 0.0f)
            {
                adsrPhase = ADSRPhase::Finished;
            }
            break;
            
        case ADSRPhase::Finished:
            adsrAmplitude = 0.0f;
            break;
    }
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

void Particle::draw (juce::Graphics& g)
{
    // Calculate base alpha from ADSR amplitude
    float lifetimeAlpha = adsrAmplitude;
    
    // Get edge crossfade info (for visual wraparound)
    const float edgeFadeZone = 50.0f;
    float distanceFromLeft = position.x - canvasBounds.getX();
    float distanceFromRight = canvasBounds.getRight() - position.x;
    
    float mainAlpha = 1.0f;
    float ghostAlpha = 0.0f;
    juce::Point<float> ghostPosition = position;
    bool shouldDrawGhost = false;
    
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
    result.amplitude = 1.0f;
    
    if (canvasBounds.getWidth() <= 0)
    {
        result.pan = 0.0f;
        return result;
    }
    
    const float edgeFadeZone = 50.0f; // 50px from edge starts fade
    float normalizedX = (position.x - canvasBounds.getX()) / canvasBounds.getWidth();
    result.pan = juce::jlimit (-1.0f, 1.0f, (normalizedX * 2.0f) - 1.0f);
    
    // Check if near left edge (x < edgeFadeZone)
    float distanceFromLeft = position.x - canvasBounds.getX();
    if (distanceFromLeft < edgeFadeZone)
    {
        // Fade amplitude down as we approach left edge
        result.amplitude = distanceFromLeft / edgeFadeZone;
        return result;
    }
    
    // Check if near right edge (x > width - edgeFadeZone)
    float distanceFromRight = canvasBounds.getRight() - position.x;
    if (distanceFromRight < edgeFadeZone)
    {
        // Fade amplitude down as we approach right edge
        result.amplitude = distanceFromRight / edgeFadeZone;
        return result;
    }
    
    // Not near any edge - full amplitude
    return result;
}

float Particle::getGrainAmplitude (const Grain& grain) const
{
    // HARDCODED 50% crossfade for all grains:
    // - First 50% of grain: fade in from 0.0 to 1.0
    // - Last 50% of grain: fade out from 1.0 to 0.0
    
    int grainPos = grain.playbackPosition;
    int halfGrain = cachedTotalGrainSamples / 2;
    
    float grainEnvelope;
    
    if (grainPos < halfGrain)
    {
        // First half: fade in from 0.0 to 1.0
        grainEnvelope = static_cast<float>(grainPos) / static_cast<float>(halfGrain);
    }
    else
    {
        // Second half: fade out from 1.0 to 0.0
        int posInSecondHalf = grainPos - halfGrain;
        int secondHalfDuration = cachedTotalGrainSamples - halfGrain;
        grainEnvelope = 1.0f - (static_cast<float>(posInSecondHalf) / static_cast<float>(secondHalfDuration));
    }
    
    // Apply particle ADSR amplitude on top of grain envelope
    return grainEnvelope * adsrAmplitude;
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
