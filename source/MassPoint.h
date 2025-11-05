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

    // Mouse handling for dragging and menu
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;
    void mouseEnter (const juce::MouseEvent& event) override;
    void mouseExit (const juce::MouseEvent& event) override;
    
    // Callback for when mass point is moved or dropped
    std::function<void()> onMassDropped;
    std::function<void()> onMassMoved;
    
    // Size/Mass controls
    float getMassMultiplier() const { return massMultiplier; }
    int getRadius() const { return radius; }
    void setRadius (int newRadius);
    
    // Static images for all vortex layers (shared across all mass points)
    static juce::Image vortexImage1;
    static juce::Image vortexImage2;
    static juce::Image vortexImage3;
    static juce::Image vortexImage4;
    
    // Static hover images for vortex layers
    static juce::Image vortexImage1Hover;
    static juce::Image vortexImage2Hover;
    static juce::Image vortexImage3Hover;
    static juce::Image vortexImage4Hover;
    
    // Set the vortex images (call once from PluginEditor)
    static void setVortexImages (const juce::Image& img1, const juce::Image& img2,
                                  const juce::Image& img3, const juce::Image& img4);
    
    // Set the hover vortex images (call once from PluginEditor)
    static void setVortexHoverImages (const juce::Image& img1Hover, const juce::Image& img2Hover,
                                       const juce::Image& img3Hover, const juce::Image& img4Hover);
    
    // Update rotation animation
    void updateRotation (float deltaTime);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MassPoint)
    juce::ComponentDragger dragger;
    juce::ComponentBoundsConstrainer constrainer;
    
    // Size states for double-click cycling (4 sizes: 50, 100, 150, 200)
    int radius = 50;
    float massMultiplier = 1.0f;
    static constexpr int minRadius = 50;
    static constexpr int maxRadius = 200;
    static constexpr int radiusStep = 50;
    
    // Rotation angles for each layer (all counter-clockwise but different speeds)
    float rotation1 = 0.0f;
    float rotation2 = 0.0f;
    float rotation3 = 0.0f;
    float rotation4 = 0.0f;
    
    // Hover state
    bool isHovered = false;
    
    // Helper to show size selection menu
    void showSizeMenu();
};
