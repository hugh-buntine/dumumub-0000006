#include "SpawnPoint.h"
#include "Logger.h"

// Initialize static members
juce::Image SpawnPoint::spawnerImage1;
juce::Image SpawnPoint::spawnerImage2;
juce::Image SpawnPoint::spawnerImage1Hover;
juce::Image SpawnPoint::spawnerImage2Hover;

//==============================================================================
SpawnPoint::SpawnPoint()
{
    setSize (20, 20);
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    momentumVector = juce::Point<float> (20.0f, 0.0f);
    LOG_INFO("SpawnPoint created");
}

SpawnPoint::~SpawnPoint()
{
    LOG_INFO("SpawnPoint destroyed");
}

void SpawnPoint::setSpawnerImages (const juce::Image& img1, const juce::Image& img2)
{
    spawnerImage1 = img1;
    spawnerImage2 = img2;
}

void SpawnPoint::setSpawnerHoverImages (const juce::Image& img1Hover, const juce::Image& img2Hover)
{
    spawnerImage1Hover = img1Hover;
    spawnerImage2Hover = img2Hover;
}

void SpawnPoint::setSelected (bool shouldBeSelected)
{
    if (selected != shouldBeSelected)
    {
        selected = shouldBeSelected;
        repaint();
        
        // Notify parent that selection changed (so arrow can be drawn/hidden)
        if (onSelectionChanged)
            onSelectionChanged();
    }
}

void SpawnPoint::updateRotation (float deltaTime)
{
    // Spawner 1 rotates clockwise at 1 radian per second
    rotation1 += 1.0f * deltaTime;
    
    // Spawner 2 rotates counter-clockwise at 1 radian per second
    rotation2 -= 1.0f * deltaTime;
}

//==============================================================================
void SpawnPoint::paint (juce::Graphics& g)
{
    float centerX = getWidth() / 2.0f;
    float centerY = getHeight() / 2.0f;
    
    const juce::Image& img1 = (selected || isHovered) ? spawnerImage1Hover : spawnerImage1;
    const juce::Image& img2 = (selected || isHovered) ? spawnerImage2Hover : spawnerImage2;
    
    // Bottom layer (counter-clockwise)
    if (img2.isValid())
    {
        float scaleX = (float)getWidth() / img2.getWidth() * 1.5f;
        float scaleY = (float)getHeight() / img2.getHeight() * 1.5f;
        
        juce::AffineTransform transform = juce::AffineTransform::translation (-img2.getWidth() / 2.0f, -img2.getHeight() / 2.0f)
            .scaled (scaleX, scaleY)
            .rotated (rotation2)
            .translated (centerX, centerY);
        
        g.drawImageTransformed (img2, transform);
    }
    
    // Top layer (clockwise)
    if (img1.isValid())
    {
        float scaleX = (float)getWidth() / img1.getWidth() * 1.5f;
        float scaleY = (float)getHeight() / img1.getHeight() * 1.5f;
        
        juce::AffineTransform transform = juce::AffineTransform::translation (-img1.getWidth() / 2.0f, -img1.getHeight() / 2.0f)
            .scaled (scaleX, scaleY)
            .rotated (rotation1)
            .translated (centerX, centerY);
        
        g.drawImageTransformed (img1, transform);
    }
    
    if (!img1.isValid() && !img2.isValid())
    {
        g.setColour (juce::Colour (0, 255, 0));
        g.fillEllipse (getLocalBounds().toFloat());
    }
}

void SpawnPoint::resized()
{
}

//==============================================================================
void SpawnPoint::mouseDown (const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu())
    {
        showMenu();
        return;
    }
    
    setSelected (true);
    dragger.startDraggingComponent (this, event);
    LOG_INFO("Started dragging SpawnPoint from (" + juce::String(getX()) + ", " + juce::String(getY()) + ")");
}

void SpawnPoint::showMenu()
{
    juce::PopupMenu menu;
    menu.setLookAndFeel (&popupMenuLookAndFeel);
    
    int spawnPointCount = getSpawnPointCount ? getSpawnPointCount() : 1;
    bool canDelete = spawnPointCount > 1;
    
    menu.addItem (1, "delete", canDelete);
    
    menu.showMenuAsync (juce::PopupMenu::Options(),
                        [this, canDelete] (int result)
                        {
                            if (result == 1 && canDelete)
                            {
                                LOG_INFO("SpawnPoint - Requesting deletion");
                                if (onDeleteRequested)
                                    onDeleteRequested();
                            }
                        });
}

void SpawnPoint::mouseDrag (const juce::MouseEvent& event)
{
    if (getParentComponent() != nullptr)
    {
        constrainer.setMinimumOnscreenAmounts (getHeight(), getWidth(), 
                                               getHeight(), getWidth());
    }
    
    dragger.dragComponent (this, event, &constrainer);
    
    if (onSpawnPointMoved)
        onSpawnPointMoved();
}

void SpawnPoint::mouseUp (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    LOG_INFO("Stopped dragging SpawnPoint at (" + juce::String(getX()) + ", " + juce::String(getY()) + ")");
}

void SpawnPoint::mouseEnter (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    isHovered = true;
    repaint();
}

void SpawnPoint::mouseExit (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    isHovered = false;
    repaint();
}
