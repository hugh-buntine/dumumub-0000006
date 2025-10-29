#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
class SpawnPoint : public juce::Component
{
public:
    SpawnPoint();
    ~SpawnPoint() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    //==============================================================================
    // Mouse handling for dragging
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;
    
    // Get/Set momentum vector (direction and magnitude)
    juce::Point<float> getMomentumVector() const { return momentumVector; }
    void setMomentumVector (const juce::Point<float>& vector) { momentumVector = vector; }
    float getMomentumMagnitude() const { return momentumVector.getDistanceFromOrigin(); }
    
    // Callback for when spawn point is moved
    std::function<void()> onSpawnPointMoved;

private:
    juce::ComponentDragger dragger;
    juce::ComponentBoundsConstrainer constrainer;
    
    // Momentum arrow data (rendering handled by Canvas)
    juce::Point<float> momentumVector { 10.0f, 0.0f }; // Direction and length from center
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpawnPoint)
};
