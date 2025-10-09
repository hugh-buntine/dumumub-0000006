#include "SpawnPoint.h"
#include "Logger.h"

//==============================================================================
SpawnPoint::SpawnPoint()
{
    // Constructor
    setSize (20, 20);
    
    // Enable mouse events
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    
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
    // Fill background with off white
    g.fillAll (juce::Colour (255, 255, 242));
    
    // Draw black outline
    g.setColour (juce::Colours::black);
    g.drawRect (getLocalBounds(), 1);
    
    // Draw component name in center
    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("SpawnPoint", getLocalBounds(), juce::Justification::centred, 1);
}

void SpawnPoint::resized()
{
    // Layout child components here
}

//==============================================================================
void SpawnPoint::mouseDown (const juce::MouseEvent& event)
{
    // Start dragging
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
}

void SpawnPoint::mouseUp (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    LOG_INFO("Stopped dragging SpawnPoint at position (" + 
             juce::String(getX()) + ", " + juce::String(getY()) + ")");
}
