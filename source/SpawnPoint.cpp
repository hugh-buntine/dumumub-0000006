#include "SpawnPoint.h"
#include "Logger.h"

//==============================================================================
SpawnPoint::SpawnPoint()
{
    // Constructor
    setSize (20, 20);
    
    // Enable mouse events
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    
    // Initialize with a default momentum vector pointing right at minimum length
    momentumVector = juce::Point<float> (20.0f, 0.0f);
    
    LOG_INFO("SpawnPoint created");
}

SpawnPoint::~SpawnPoint()
{
    // Destructor
    LOG_INFO("SpawnPoint destroyed");
}

//==============================================================================
void SpawnPoint::paint (juce::Graphics& g)
{
    // Draw green circle
    g.setColour (juce::Colour (0, 255, 0));
    g.fillEllipse (getLocalBounds().toFloat());
    
}

void SpawnPoint::resized()
{
    // Layout child components here
}

//==============================================================================
void SpawnPoint::mouseDown (const juce::MouseEvent& event)
{
    // Start dragging the spawn point
    dragger.startDraggingComponent (this, event);
    LOG_INFO("Started dragging SpawnPoint from position (" + 
             juce::String(getX()) + ", " + juce::String(getY()) + ")");
}

void SpawnPoint::mouseDrag (const juce::MouseEvent& event)
{
    // Constrain dragging to parent component bounds if parent exists
    if (getParentComponent() != nullptr)
    {
        constrainer.setMinimumOnscreenAmounts (getHeight(), getWidth(), 
                                               getHeight(), getWidth());
    }
    
    // Perform the drag
    dragger.dragComponent (this, event, &constrainer);
    
    // Notify parent that spawn point moved (so arrow can be redrawn)
    if (onSpawnPointMoved)
        onSpawnPointMoved();
}

void SpawnPoint::mouseUp (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    LOG_INFO("Stopped dragging SpawnPoint at position (" + 
             juce::String(getX()) + ", " + juce::String(getY()) + ")");
}
