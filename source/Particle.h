#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
class Particle
{
public:
    Particle (juce::Point<float> position, juce::Point<float> velocity);
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
    
    // Draw the particle
    void draw (juce::Graphics& g);

private:
    juce::Point<float> position;
    juce::Point<float> velocity;
    juce::Point<float> acceleration;
    float lifeTime = 0.0f;
    float maxLifeTime = 30.0f; // 30 seconds before particle dies
    float radius = 3.0f;
};
