#include "Canvas.h"
#include "Logger.h"

//==============================================================================
Canvas::Canvas()
{
    // Constructor
    LOG_INFO("Canvas created");
    setSize (400, 400);
    newSpawnPoint();
}

Canvas::~Canvas()
{
    // Destructor
    LOG_INFO("Canvas destroyed - had " + juce::String(spawnPoints.size()) + " spawn points");
}

//==============================================================================
void Canvas::paint (juce::Graphics& g)
{
    // Fill background with off white
    g.fillAll (juce::Colour (255, 255, 242));
    
    // Draw black outline
    g.setColour (juce::Colours::black);
    g.drawRect (getLocalBounds(), 1);
    
    // Draw component name in center
    g.setColour (juce::Colours::black);
    g.setFont (15.0f);
    g.drawFittedText ("Canvas", getLocalBounds(), juce::Justification::centred, 1);
}

void Canvas::resized()
{
    // Layout child components here
}

void Canvas::newSpawnPoint()
{
    if (spawnPoints.size() < maxSpawnPoints)
    {
        auto* newPoint = new SpawnPoint();
        spawnPoints.add (newPoint);
        addAndMakeVisible (newPoint);
        
        // Place at random position on canvas
        auto bounds = getLocalBounds();
        if (bounds.getWidth() > 0 && bounds.getHeight() > 0)
        {
            juce::Random random;
            int x = random.nextInt (bounds.getWidth());
            int y = random.nextInt (bounds.getHeight());
            newPoint->setCentrePosition (x, y);
            LOG_INFO("Created new spawn point at (" + juce::String(x) + ", " + juce::String(y) + ")");
        }
        else
        {
            LOG_WARNING("Canvas bounds not yet set - spawn point placed at origin");
        }
    }
    else
    {
        LOG_WARNING("Maximum spawn points reached (" + juce::String(maxSpawnPoints) + ")");
    }
}
