#include "Particle.h"
#include "Logger.h"

//==============================================================================
Particle::Particle (juce::Point<float> pos, juce::Point<float> vel)
    : position (pos), velocity (vel)
{
    LOG_INFO("Particle created at (" + juce::String(pos.x) + ", " + juce::String(pos.y) + ")");
}

Particle::~Particle()
{
}

//==============================================================================
void Particle::update (float deltaTime)
{
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
    // Calculate alpha based on remaining lifetime (fade out as approaching death)
    float lifetimeRatio = lifeTime / maxLifeTime;
    float alpha = 1.0f - lifetimeRatio; // 1.0 at birth, 0.0 at death
    
    // Draw particle as a small circle with alpha
    g.setColour (juce::Colours::blue.withAlpha (alpha));
    g.fillEllipse (position.x - radius, position.y - radius, radius * 2, radius * 2);
    
    // Draw outline (also fades)
    g.setColour (juce::Colours::white.withAlpha (alpha));
    g.drawEllipse (position.x - radius, position.y - radius, radius * 2, radius * 2, 1.0f);
}
