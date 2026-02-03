#include "Particle.h"
#include "Logger.h"

juce::Image Particle::starImage;
std::vector<float> Particle::hannWindowTable;

void Particle::initializeHannTable()
{
    if (!hannWindowTable.empty())
        return;
    
    hannWindowTable.resize (HANN_TABLE_SIZE);
    
    const float pi = juce::MathConstants<float>::pi;
    for (size_t i = 0; i < HANN_TABLE_SIZE; ++i)
    {
        float normalizedPos = static_cast<float>(i) / static_cast<float>(HANN_TABLE_SIZE - 1);
        hannWindowTable[i] = 0.5f * (1.0f - std::cos (2.0f * pi * normalizedPos));
    }
}

float Particle::getHannWindowValue (float normalizedPosition)
{
    if (hannWindowTable.empty())
        initializeHannTable();
    
    normalizedPosition = juce::jlimit (0.0f, 1.0f, normalizedPosition);
    
    float tablePos = normalizedPosition * static_cast<float>(HANN_TABLE_SIZE - 1);
    size_t index0 = static_cast<size_t>(tablePos);
    size_t index1 = juce::jmin (index0 + 1, static_cast<size_t>(HANN_TABLE_SIZE - 1));
    
    float frac = tablePos - static_cast<float>(index0);
    float value = hannWindowTable[index0] + frac * (hannWindowTable[index1] - hannWindowTable[index0]);
    
    // Flush denormals to prevent noise artifacts
    if (std::abs(value) < 1e-15f)
        value = 0.0f;
    
    return value;
}

//==============================================================================
Particle::Particle (juce::Point<float> initialPosition, juce::Point<float> initialVelocity, 
                    const juce::Rectangle<float>& bounds, int noteNumber,
                    float attack, float sustain, float sustainLinear, float release,
                    float velocityMultiplier, float pitch)
    : position (initialPosition), velocity (initialVelocity), 
      lifeTime (0.0f), 
      midiNoteNumber (noteNumber),
      adsrPhase (ADSRPhase::Attack),
      adsrTime (0.0f),
      attackTime (attack),
      sustainLevel (sustain),
      sustainLevelLinear (sustainLinear),
      releaseTime (release),
      adsrAmplitude (0.0f),
      adsrAmplitudeLinear (0.0f),
      initialVelocityMultiplier (velocityMultiplier), pitchShift (pitch),
      canvasBounds (bounds),
      currentSampleRate (0.0), 
      samplesSinceLastGrainTrigger (0),
      cachedTotalGrainSamples (2205),
      lastPosition (initialPosition)
{
    activeGrains.reserve (32);
}

void Particle::triggerNewGrain (int bufferLength)
{
    // Voice stealing when at max grains
    if (activeGrains.size() >= MAX_GRAINS_PER_PARTICLE)
    {
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
        
        activeGrains.erase (activeGrains.begin() + oldestGrainIndex);
        
        static int voiceStealCount = 0;
        voiceStealCount++;
        if (voiceStealCount % 50 == 1)
        {
            LOG_WARNING("Voice stealing: max grains reached, removed oldest grain");
        }
    }
    
    int startSample = calculateGrainStartPosition (bufferLength);
    activeGrains.push_back (Grain (startSample, cachedTotalGrainSamples));
}

void Particle::updateGrains (int numSamples)
{
    samplesSinceLastGrainTrigger += numSamples;
    
    // Update each grain by actual samples rendered, not buffer size
    for (auto& grain : activeGrains)
    {
        int advanceAmount = (grain.samplesRenderedThisBuffer > 0) 
                          ? grain.samplesRenderedThisBuffer 
                          : numSamples;
        
        grain.playbackPosition += advanceAmount;
        
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
            if (attackTime > 0.0f)
            {
                float linearProgress = juce::jmin (1.0f, adsrTime / attackTime);
                adsrAmplitude = linearProgress * linearProgress;
                adsrAmplitudeLinear = adsrAmplitude;
            }
            else
            {
                adsrAmplitude = 1.0f;
                adsrAmplitudeLinear = 1.0f;
            }
            
            if (adsrAmplitude >= 1.0f)
            {
                adsrPhase = ADSRPhase::Decay;
                adsrTime = 0.0f;
            }
            break;
            
        case ADSRPhase::Decay:
            if (decayTime > 0.0f)
            {
                float linearProgress = juce::jmin (1.0f, adsrTime / decayTime);
                float curve = 1.0f - std::pow (1.0f - linearProgress, 2.0f);
                
                adsrAmplitude = 1.0f - (curve * (1.0f - sustainLevel));
                adsrAmplitude = juce::jmax (sustainLevel, adsrAmplitude);
                
                adsrAmplitudeLinear = 1.0f - (curve * (1.0f - sustainLevelLinear));
                adsrAmplitudeLinear = juce::jmax (sustainLevelLinear, adsrAmplitudeLinear);
            }
            else
            {
                adsrAmplitude = sustainLevel;
                adsrAmplitudeLinear = sustainLevelLinear;
            }
            
            if (adsrTime >= decayTime)
            {
                adsrPhase = ADSRPhase::Sustain;
                adsrAmplitude = sustainLevel;
                adsrAmplitudeLinear = sustainLevelLinear;
                adsrTime = 0.0f;
            }
            break;
            
        case ADSRPhase::Sustain:
            adsrAmplitude = sustainLevel;
            adsrAmplitudeLinear = sustainLevelLinear;
            break;
            
        case ADSRPhase::Release:
        {
            // Add grain fade duration to release time to prevent clicks
            float grainFadeDuration = 0.010f;
            float effectiveReleaseTime = releaseTime + grainFadeDuration;
            
            float linearProgress = juce::jmin (1.0f, adsrTime / effectiveReleaseTime);
            float curve = std::pow (1.0f - linearProgress, 4.0f);
            
            adsrAmplitude = releaseStartAmplitude * curve;
            adsrAmplitude = juce::jmax (0.0f, adsrAmplitude);
            
            adsrAmplitudeLinear = releaseStartAmplitudeLinear * curve;
            adsrAmplitudeLinear = juce::jmax (0.0f, adsrAmplitudeLinear);
            
            if (adsrAmplitude <= 0.0f)
            {
                adsrPhase = ADSRPhase::Finished;
            }
            break;
        }
            
        case ADSRPhase::Finished:
            adsrAmplitude = 0.0f;
            adsrAmplitudeLinear = 0.0f;
            break;
    }
}

void Particle::updateADSRSample (double sampleRate)
{
    if (sampleRate <= 0.0)
        return;
    
    float deltaTime = static_cast<float>(1.0 / sampleRate);
    updateADSR (deltaTime);
    
    // One-pole lowpass smoothing to eliminate stepping artifacts
    float smoothingCoeff = 1.0f - std::exp(-2.2f / (0.0005f * static_cast<float>(sampleRate)));
    adsrAmplitudeSmoothed += smoothingCoeff * (adsrAmplitude - adsrAmplitudeSmoothed);
}

void Particle::triggerRelease()
{
    if (adsrPhase == ADSRPhase::Attack || adsrPhase == ADSRPhase::Decay || adsrPhase == ADSRPhase::Sustain)
    {
        releaseStartAmplitude = adsrAmplitude;
        releaseStartAmplitudeLinear = adsrAmplitudeLinear;
        adsrPhase = ADSRPhase::Release;
        adsrTime = 0.0f;
    }
}

//==============================================================================
void Particle::update (float deltaTime)
{
    updateADSR (deltaTime);
    
    if (justWrappedAround)
    {
        wraparoundSmoothingTime += deltaTime;
        if (wraparoundSmoothingTime >= wraparoundSmoothDuration)
        {
            justWrappedAround = false;
            wraparoundSmoothingTime = 0.0f;
        }
    }
    
    // Trail system
    if (trail.empty() || position.getDistanceFrom(trail.back().position) > 2.0f)
    {
        TrailPoint newPoint;
        newPoint.position = position;
        newPoint.age = 0.0f;
        trail.push_back (newPoint);
        
        if (trail.size() > maxTrailPoints)
            trail.erase (trail.begin());
    }
    
    for (auto& point : trail)
        point.age += deltaTime;
    
    trail.erase (
        std::remove_if (trail.begin(), trail.end(),
                       [](const TrailPoint& p) { return p.age > trailFadeTime; }),
        trail.end()
    );
    
    lastPosition = position;
    velocity += acceleration * deltaTime;
    position += velocity * deltaTime;
    acceleration = juce::Point<float> (0.0f, 0.0f);
    
    lifeTime += deltaTime;
}

void Particle::applyForce (const juce::Point<float>& force)
{
    acceleration += force;
}

void Particle::wrapAround (const juce::Rectangle<float>& bounds)
{
    if (position.x < bounds.getX())
        position.x = bounds.getRight();
    else if (position.x > bounds.getRight())
        position.x = bounds.getX();
    
    if (position.y < bounds.getY())
        position.y = bounds.getBottom();
    else if (position.y > bounds.getBottom())
        position.y = bounds.getY();
}

void Particle::bounceOff (const juce::Rectangle<float>& bounds)
{
    if (position.x < bounds.getX())
    {
        position.x = bounds.getX();
        velocity.x = std::abs(velocity.x);
    }
    else if (position.x > bounds.getRight())
    {
        position.x = bounds.getRight();
        velocity.x = -std::abs(velocity.x);
    }
    
    if (position.y < bounds.getY())
    {
        position.y = bounds.getY();
        velocity.y = std::abs(velocity.y);
    }
    else if (position.y > bounds.getBottom())
    {
        position.y = bounds.getBottom();
        velocity.y = -std::abs(velocity.y);
    }
}

void Particle::draw (juce::Graphics& g)
{
    // Use linear ADSR for visuals (matches slider position)
    float lifetimeAlpha = adsrAmplitudeLinear;
    
    // Edge crossfade for visual wraparound
    const float edgeFadeZone = 50.0f;
    float distanceFromLeft = position.x - canvasBounds.getX();
    float distanceFromRight = canvasBounds.getRight() - position.x;
    
    float mainAlpha = 1.0f;
    float ghostAlpha = 0.0f;
    juce::Point<float> ghostPosition = position;
    bool shouldDrawGhost = false;
    
    if (!bounceMode)
    {
        if (distanceFromLeft < edgeFadeZone && canvasBounds.getWidth() > 0)
        {
            mainAlpha = distanceFromLeft / edgeFadeZone;
            ghostAlpha = 1.0f - mainAlpha;
            ghostPosition.x = position.x + canvasBounds.getWidth();
            shouldDrawGhost = true;
        }
        else if (distanceFromRight < edgeFadeZone && canvasBounds.getWidth() > 0)
        {
            mainAlpha = distanceFromRight / edgeFadeZone;
            ghostAlpha = 1.0f - mainAlpha;
            ghostPosition.x = position.x - canvasBounds.getWidth();
            shouldDrawGhost = true;
        }
    }
    
    juce::Colour trailColor (255, 255, 242);
    
    auto drawTrailFrom = [&](juce::Point<float> offset, float alpha)
    {
        if (trail.size() > 1)
        {
            for (size_t i = 0; i < trail.size() - 1; ++i)
            {
                const auto& p1 = trail[i];
                const auto& p2 = trail[i + 1];
                
                // Skip teleported segments
                float dx = p2.position.x - p1.position.x;
                float dy = p2.position.y - p1.position.y;
                float distanceSquared = dx * dx + dy * dy;
                if (distanceSquared > 100.0f * 100.0f)
                    continue;
                
                float trailFade = 1.0f - (p1.age / trailFadeTime);
                float finalAlpha = lifetimeAlpha * alpha * trailFade * 0.6f;
                
                g.setColour (trailColor.withAlpha (finalAlpha));
                g.drawLine (p1.position.x + offset.x, p1.position.y + offset.y, 
                           p2.position.x + offset.x, p2.position.y + offset.y, 
                           2.0f);
            }
        }
    };
    
    auto drawStarAt = [&](juce::Point<float> pos, float alpha)
    {
        float combinedAlpha = lifetimeAlpha * alpha;
        
        if (starImage.isValid())
        {
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
            juce::Colour particleColor = activeGrains.empty()
                ? juce::Colours::blue
                : juce::Colours::red;
            g.setColour (particleColor.withAlpha (combinedAlpha));
            g.fillEllipse (pos.x - radius, pos.y - radius, radius * 2, radius * 2);
        }
    };
    
    drawTrailFrom (juce::Point<float>(0, 0), mainAlpha);
    drawStarAt (position, mainAlpha);
    
    if (shouldDrawGhost && ghostAlpha > 0.0f)
    {
        juce::Point<float> ghostOffset (ghostPosition.x - position.x, 0);
        drawTrailFrom (ghostOffset, ghostAlpha);
        drawStarAt (ghostPosition, ghostAlpha);
    }
}

//==============================================================================
void Particle::updateSampleRate (double sampleRate)
{
    if (std::abs(sampleRate - currentSampleRate) > 0.001 && sampleRate > 0)
    {
        currentSampleRate = sampleRate;
        cachedTotalGrainSamples = static_cast<int>((grainSizeMs / 1000.0f) * sampleRate);
        
        // 50% crossfade
        int halfGrainSamples = cachedTotalGrainSamples / 2;
        cachedAttackSamples = halfGrainSamples;
        cachedReleaseSamples = halfGrainSamples;
    }
}

void Particle::setGrainParameters (float grainSizeMsNew, float /*attackPercentNew*/, float /*releasePercentNew*/)
{
    if (std::abs(grainSizeMs - grainSizeMsNew) > 0.001f)
    {
        grainSizeMs = grainSizeMsNew;
        cachedTotalGrainSamples = static_cast<int>((grainSizeMs / 1000.0f) * currentSampleRate);
    }
}

bool Particle::shouldTriggerNewGrain (double sampleRate, float grainFrequencyHz)
{
    if (isFirstGrain)
    {
        isFirstGrain = false;
        samplesSinceLastGrainTrigger = 0;
        return true;
    }
    
    int samplesPerPeriod = static_cast<int>(sampleRate / grainFrequencyHz);
    
    if (samplesSinceLastGrainTrigger >= samplesPerPeriod)
    {
        samplesSinceLastGrainTrigger = 0;
        return true;
    }
    
    return false;
}

float Particle::getPan() const
{
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
    
    const float edgeFadeZone = 50.0f;
    float normalizedX = (position.x - canvasBounds.getX()) / canvasBounds.getWidth();
    float basePan = juce::jlimit (-1.0f, 1.0f, (normalizedX * 2.0f) - 1.0f);
    
    // In bounce mode, allow full left/right panning
    if (bounceMode)
    {
        result.pan = basePan;
        return result;
    }
    
    // In wrap mode, transition to center at edges for seamless wraparound
    float distanceFromLeft = position.x - canvasBounds.getX();
    if (distanceFromLeft < edgeFadeZone)
    {
        float edgeFactor = distanceFromLeft / edgeFadeZone;
        result.pan = basePan * edgeFactor;
        return result;
    }
    
    float distanceFromRight = canvasBounds.getRight() - position.x;
    if (distanceFromRight < edgeFadeZone)
    {
        float edgeFactor = distanceFromRight / edgeFadeZone;
        result.pan = basePan * edgeFactor;
        return result;
    }
    
    result.pan = basePan;
    return result;
}

float Particle::getGrainAmplitude (const Grain& grain) const
{
    // Hann window for smooth grain envelope
    if (grain.totalSamples <= 0)
        return 0.0f;
    
    int grainPos = grain.playbackPosition;
    
    // Past grain end = fully faded out
    if (grainPos >= cachedTotalGrainSamples)
        return 0.0f;
    
    // Fixed-duration fade-in/fade-out (10ms) regardless of grain size
    const int fadeSamples = static_cast<int>(0.010f * currentSampleRate);
    const int grainTotalSamples = grain.totalSamples;
    const int halfGrain = grainTotalSamples / 2;
    
    // For short grains, cap fade to half grain size to prevent overlap
    const int actualFadeInSamples = juce::jmin(fadeSamples, halfGrain);
    const int actualFadeOutSamples = juce::jmin(fadeSamples, halfGrain);
    
    float grainEnvelope = 1.0f;
    
    if (grainPos < actualFadeInSamples)
    {
        // Fade in: first half of Hann curve (0.0 → 1.0)
        float fadeProgress = static_cast<float>(grainPos) / static_cast<float>(actualFadeInSamples - 1);
        fadeProgress = juce::jlimit (0.0f, 1.0f, fadeProgress);
        float hannPos = fadeProgress * 0.5f;
        grainEnvelope = getHannWindowValue(hannPos);
    }
    else if (grainPos >= grainTotalSamples - actualFadeOutSamples)
    {
        // Fade out: second half of Hann curve (1.0 → 0.0)
        int samplesToEnd = grainTotalSamples - grainPos;
        float fadeProgress = 1.0f - (static_cast<float>(samplesToEnd - 1) / static_cast<float>(actualFadeOutSamples - 1));
        fadeProgress = juce::jlimit (0.0f, 1.0f, fadeProgress);
        float hannPos = 0.5f + (fadeProgress * 0.5f);
        grainEnvelope = getHannWindowValue(hannPos);
    }
    
    if (!std::isfinite(grainEnvelope))
        grainEnvelope = 0.0f;
    
    return grainEnvelope;
}

int Particle::calculateGrainStartPosition (int bufferLength)
{
    if (bufferLength <= 0 || canvasBounds.getHeight() <= 0)
        return 0;
    
    // Y position maps to audio buffer position
    // Y=0 (top) = end of sample, Y=height (bottom) = start of sample
    float normalizedY = 1.0f - (position.y / canvasBounds.getHeight());
    normalizedY = juce::jlimit (0.0f, 1.0f, normalizedY);
    
    int startSample = static_cast<int>(normalizedY * bufferLength);
    return juce::jlimit (0, bufferLength - 1, startSample);
}
