#include "MassPoint.h"
#include "Logger.h"

//==============================================================================
MassPoint::MassPoint()
{
    // Constructor
    setSize (radius, radius);
    
    // Enable mouse events
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    
    LOG_INFO("MassPoint created with radius " + juce::String(radius));
}

MassPoint::~MassPoint()
{
    // Destructor
    LOG_INFO("MassPoint destroyed");
}

//==============================================================================
void MassPoint::paint (juce::Graphics& g)
{
    // Draw black circle (size based on radius)
    g.setColour (juce::Colours::black);
    g.fillEllipse (getLocalBounds().toFloat());
    
    // Draw white outline for visibility
    g.setColour (juce::Colours::white);
    g.drawEllipse (getLocalBounds().toFloat(), 2.0f);
}

void MassPoint::resized()
{
    // Layout child components here
}

//==============================================================================
void MassPoint::mouseDown (const juce::MouseEvent& event)
{
    // Check for multiple clicks (2+) to increase size
    int numClicks = event.getNumberOfClicks();
    
    if (numClicks >= 2)
    {
        // Each click from 2 onwards increases size by one step
        int stepsToIncrease = numClicks - 1; // 2 clicks = 1 step, 3 clicks = 2 steps, etc.
        
        for (int i = 0; i < stepsToIncrease; ++i)
        {
            radius += radiusStep;
            if (radius > maxRadius)
                radius = minRadius;
        }
        
        // Update mass multiplier based on radius (larger = stronger gravity)
        massMultiplier = static_cast<float>(radius) / static_cast<float>(minRadius);
        
        // Resize component (keep centered)
        auto oldBounds = getBounds();
        auto oldCenter = oldBounds.getCentre();
        setSize (radius, radius);
        setCentrePosition (oldCenter);
        
        LOG_INFO(juce::String(numClicks) + " clicks - MassPoint size changed to radius " + 
                 juce::String(radius) + " with mass multiplier " + juce::String(massMultiplier));
        
        // Trigger repaint for gravity waves
        if (onMassMoved)
            onMassMoved();
        
        return;
    }
    
    // Start dragging (single click)
    dragger.startDraggingComponent (this, event);
    LOG_INFO("Started dragging MassPoint from position (" + 
             juce::String(getX()) + ", " + juce::String(getY()) + ")");
}

void MassPoint::mouseDrag (const juce::MouseEvent& event)
{
    // Constrain dragging to parent component bounds if parent exists
    if (getParentComponent() != nullptr)
    {
        constrainer.setMinimumOnscreenAmounts (getHeight(), getWidth(), 
                                               getHeight(), getWidth());
    }
    
    // Perform the drag
    dragger.dragComponent (this, event, &constrainer);
    
    // Trigger callback while mass is being moved
    if (onMassMoved)
        onMassMoved();
}

void MassPoint::mouseUp (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    LOG_INFO("Stopped dragging MassPoint at position (" + 
             juce::String(getX()) + ", " + juce::String(getY()) + ")");
    
    // Trigger callback when mass is dropped
    if (onMassDropped)
        onMassDropped();
}
