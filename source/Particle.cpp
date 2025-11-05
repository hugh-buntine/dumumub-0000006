#include "Particle.h"
#include "Logger.h"

// Initialize static member
juce::Image Particle::starImage;

//==============================================================================
Particle::Particle (juce::Point<float> position, juce::Point<float> velocity, 
                    const juce::Rectangle<float>& canvasBounds, float maxLifeTime,
                    float initialVelocity, float pitchShift)
    : position (position), velocity (velocity), canvasBounds (canvasBounds),
      lifeTime (0.0f), maxLifeTime (maxLifeTime), 
      initialVelocityMultiplier (initialVelocity), pitchShift (pitchShift),
      currentSampleRate (0.0), samplesSinceLastGrainTrigger (0),
      cachedTotalGrainSamples (2205), cachedAttackSamples (220), cachedReleaseSamples (220)
{
    // Reserve space for grains to avoid allocations during audio processing
    // With 100Hz grain frequency, we might have ~20 overlapping grains at most
    activeGrains.reserve (32);
}

void Particle::triggerNewGrain (int bufferLength)
{
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
void Particle::update (float deltaTime)
{
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
    // Calculate base alpha from lifetime (particle fades as it dies)
    float lifetimeAlpha = juce::jmax (0.0f, 1.0f - (lifeTime / maxLifeTime));
    
    // Trail color is always off-white (#FFFFF2 = RGB 255, 255, 242)
    juce::Colour trailColor (255, 255, 242);
    
    // Draw trail (from oldest to newest)
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
            
            // Calculate alpha: base transparency from particle lifetime, 
            // then fade as trail point ages
            float trailFade = 1.0f - (p1.age / trailFadeTime);
            float alpha = lifetimeAlpha * trailFade * 0.6f; // 0.6 makes trail dimmer than particle
            
            // Draw line segment with off-white color
            g.setColour (trailColor.withAlpha (alpha));
            g.drawLine (p1.position.x, p1.position.y, 
                       p2.position.x, p2.position.y, 
                       2.0f); // Line thickness
        }
    }
    
    // Draw the particle itself using star image if available
    if (starImage.isValid())
    {
        // Draw 15x15 star image centered on particle position with fade
        float starSize = 15.0f;
        g.setOpacity (lifetimeAlpha);
        g.drawImage (starImage, 
                    juce::Rectangle<float> (position.x - starSize / 2, 
                                           position.y - starSize / 2, 
                                           starSize, starSize),
                    juce::RectanglePlacement::fillDestination);
    }
    else
    {
        // Fallback to drawing a circle if star image not loaded
        juce::Colour particleColor = activeGrains.empty()
            ? juce::Colours::blue   // Blue when silent
            : juce::Colours::red;   // Red while playing
        g.setColour (particleColor.withAlpha (lifetimeAlpha));
        g.fillEllipse (position.x - radius, position.y - radius, radius * 2, radius * 2);
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
        
        // Calculate attack/release samples as percentage of half grain
        int halfGrainSamples = cachedTotalGrainSamples / 2;
        cachedAttackSamples = static_cast<int>((attackPercent / 100.0f) * halfGrainSamples);
        cachedReleaseSamples = static_cast<int>((releasePercent / 100.0f) * halfGrainSamples);
        
        // Logging disabled in audio thread to prevent dropouts
        // LOG_INFO("Particle sample rate updated: " + juce::String(sampleRate) + " Hz, " +
        //          "grain samples: " + juce::String(cachedTotalGrainSamples));
    }
}

void Particle::setGrainParameters (float grainSizeMsNew, float attackPercentNew, float releasePercentNew)
{
    // Only update if values changed
    if (grainSizeMs != grainSizeMsNew || attackPercent != attackPercentNew || releasePercent != releasePercentNew)
    {
        grainSizeMs = grainSizeMsNew;
        attackPercent = attackPercentNew;
        releasePercent = releasePercentNew;
        
        // Recalculate cached values
        cachedTotalGrainSamples = static_cast<int>((grainSizeMs / 1000.0f) * currentSampleRate);
        
        // Calculate attack/release samples as percentage of half grain
        int halfGrainSamples = cachedTotalGrainSamples / 2;
        cachedAttackSamples = static_cast<int>((attackPercent / 100.0f) * halfGrainSamples);
        cachedReleaseSamples = static_cast<int>((releasePercent / 100.0f) * halfGrainSamples);
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

Particle::EdgeCrossfade Particle::getEdgeCrossfade() const
{
    EdgeCrossfade result;
    result.crossfadeAmount = 0.0f;
    
    if (canvasBounds.getWidth() <= 0)
    {
        result.mainPan = 0.0f;
        result.crossfadePan = 0.0f;
        return result;
    }
    
    const float edgeFadeZone = 20.0f; // 20px from edge starts crossfade
    float normalizedX = (position.x - canvasBounds.getX()) / canvasBounds.getWidth();
    result.mainPan = juce::jlimit (-1.0f, 1.0f, (normalizedX * 2.0f) - 1.0f);
    
    // Check if near left edge (x < edgeFadeZone)
    float distanceFromLeft = position.x - canvasBounds.getX();
    if (distanceFromLeft < edgeFadeZone)
    {
        // Fade to right side
        result.crossfadeAmount = 1.0f - (distanceFromLeft / edgeFadeZone);
        result.crossfadePan = 1.0f; // Right side
        return result;
    }
    
    // Check if near right edge (x > width - edgeFadeZone)
    float distanceFromRight = canvasBounds.getRight() - position.x;
    if (distanceFromRight < edgeFadeZone)
    {
        // Fade to left side
        result.crossfadeAmount = 1.0f - (distanceFromRight / edgeFadeZone);
        result.crossfadePan = -1.0f; // Left side
        return result;
    }
    
    // Not near any edge
    result.crossfadePan = result.mainPan;
    return result;
}

float Particle::getGrainAmplitude (const Grain& grain) const
{
    int grainPos = grain.playbackPosition;
    int halfGrain = cachedTotalGrainSamples / 2;
    
    // First half of grain - attack phase
    if (grainPos < halfGrain)
    {
        // Attack fades in during the first cachedAttackSamples of the first half
        if (grainPos < cachedAttackSamples)
        {
            return static_cast<float>(grainPos) / cachedAttackSamples;
        }
        // Rest of first half is full volume
        return 1.0f;
    }
    
    // Second half of grain - release phase
    else
    {
        int posInSecondHalf = grainPos - halfGrain;
        int secondHalfDuration = cachedTotalGrainSamples - halfGrain;
        
        // Sustain at full volume for beginning of second half
        if (posInSecondHalf < secondHalfDuration - cachedReleaseSamples)
        {
            return 1.0f;
        }
        
        // Release fades out during the last cachedReleaseSamples of the second half
        int samplesFromEnd = secondHalfDuration - posInSecondHalf;
        return static_cast<float>(samplesFromEnd) / cachedReleaseSamples;
    }
}

float Particle::getLifetimeAmplitude() const
{
    // Fade out over the last 5 seconds of life using logarithmic curve
    float fadeOutTime = 5.0f; // seconds
    
    if (maxLifeTime - lifeTime < fadeOutTime)
    {
        // Fading out - use logarithmic curve for perceptual linearity
        float fadeProgress = (maxLifeTime - lifeTime) / fadeOutTime; // 1.0 to 0.0
        
        // Convert to dB scale: 0dB at full life, -60dB at death
        float fadeDb = fadeProgress * 60.0f - 60.0f; // -60 to 0 dB
        float fadeLinear = juce::Decibels::decibelsToGain (fadeDb);
        
        return juce::jmax (0.0f, fadeLinear);
    }
    
    // Full volume for most of life
    return 1.0f;
}

int Particle::calculateGrainStartPosition (int bufferLength)
{
    if (bufferLength <= 0 || canvasBounds.getHeight() <= 0)
        return 0;
    
    // Calculate grain start based on current Y position
    // Y=0 (top) = start of sample (0.0), Y=height (bottom) = end of sample (1.0)
    float normalizedY = position.y / canvasBounds.getHeight();
    normalizedY = juce::jlimit (0.0f, 1.0f, normalizedY);
    
    int startSample = static_cast<int>(normalizedY * bufferLength);
    return juce::jlimit (0, bufferLength - 1, startSample);
}
