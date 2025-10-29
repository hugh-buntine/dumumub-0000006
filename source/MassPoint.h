#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
class MassPoint : public juce::Component
{
public:
    MassPoint();
    ~MassPoint() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    // Mouse handling for dragging
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;
    
    // Callback for when mass point is moved or dropped
    std::function<void()> onMassDropped;
    std::function<void()> onMassMoved;
    
    // Size/Mass controls
    float getMassMultiplier() const { return massMultiplier; }
    int getRadius() const { return radius; }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MassPoint)
    juce::ComponentDragger dragger;
    juce::ComponentBoundsConstrainer constrainer;
    
    // Size states for double-click cycling
    int radius = 20;
    float massMultiplier = 1.0f;
    static constexpr int minRadius = 20;
    static constexpr int maxRadius = 50;
    static constexpr int radiusStep = 10;
};
