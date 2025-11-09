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
    // Mouse handling for dragging, selection, and menu
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;
    void mouseEnter (const juce::MouseEvent& event) override;
    void mouseExit (const juce::MouseEvent& event) override;
    
    // Get/Set momentum vector (direction and magnitude)
    juce::Point<float> getMomentumVector() const { return momentumVector; }
    void setMomentumVector (const juce::Point<float>& vector) { momentumVector = vector; }
    float getMomentumMagnitude() const { return momentumVector.getDistanceFromOrigin(); }
    
    // Selection state
    bool isSelected() const { return selected; }
    void setSelected (bool shouldBeSelected);
    
    // Callback for when spawn point is moved
    std::function<void()> onSpawnPointMoved;
    
    // Callback for when selection state changes
    std::function<void()> onSelectionChanged;
    
    // Callback for when delete is requested
    std::function<void()> onDeleteRequested;
    
    // Callback to get the total number of spawn points (for menu state)
    std::function<int()> getSpawnPointCount;
    
    // Set the spawner images for all spawn points to use
    static void setSpawnerImages (const juce::Image& img1, const juce::Image& img2);
    
    // Set the hover spawner images for all spawn points to use
    static void setSpawnerHoverImages (const juce::Image& img1Hover, const juce::Image& img2Hover);
    
    // Update rotation (called by timer)
    void updateRotation (float deltaTime);

private:
    juce::ComponentDragger dragger;
    juce::ComponentBoundsConstrainer constrainer;
    
    // Momentum arrow data (rendering handled by Canvas)
    juce::Point<float> momentumVector { 10.0f, 0.0f }; // Direction and length from center
    
    // Static spawner images shared by all spawn points
    static juce::Image spawnerImage1;
    static juce::Image spawnerImage2;
    
    // Static hover spawner images
    static juce::Image spawnerImage1Hover;
    static juce::Image spawnerImage2Hover;
    
    // Rotation angles in radians for both layers
    float rotation1 = 0.0f;  // Clockwise rotation
    float rotation2 = 0.0f;  // Counter-clockwise rotation
    
    // Hover and selection state
    bool isHovered = false;
    bool selected = false;
    
    // Helper to show menu
    void showMenu();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpawnPoint)
};
