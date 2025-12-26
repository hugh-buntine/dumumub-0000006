#include "MassPoint.h"
#include "Logger.h"

// Initialize static members
juce::Image MassPoint::vortexImage1;
juce::Image MassPoint::vortexImage2;
juce::Image MassPoint::vortexImage3;
juce::Image MassPoint::vortexImage4;
juce::Image MassPoint::vortexImage1Hover;
juce::Image MassPoint::vortexImage2Hover;
juce::Image MassPoint::vortexImage3Hover;
juce::Image MassPoint::vortexImage4Hover;

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

void MassPoint::setVortexImages (const juce::Image& img1, const juce::Image& img2,
                                  const juce::Image& img3, const juce::Image& img4)
{
    vortexImage1 = img1;
    vortexImage2 = img2;
    vortexImage3 = img3;
    vortexImage4 = img4;
}

void MassPoint::setVortexHoverImages (const juce::Image& img1Hover, const juce::Image& img2Hover,
                                       const juce::Image& img3Hover, const juce::Image& img4Hover)
{
    vortexImage1Hover = img1Hover;
    vortexImage2Hover = img2Hover;
    vortexImage3Hover = img3Hover;
    vortexImage4Hover = img4Hover;
}

void MassPoint::updateRotation (float deltaTime)
{
    // All layers rotate counter-clockwise at different speeds (radians per second)
    rotation1 -= 0.5f * deltaTime;  // Slowest
    rotation2 -= 1.0f * deltaTime;  // Medium-slow
    rotation3 -= 1.5f * deltaTime;  // Medium-fast
    rotation4 -= 2.0f * deltaTime;  // Fastest
}

//==============================================================================
void MassPoint::paint (juce::Graphics& g)
{
    // Get center position
    float centerX = getWidth() / 2.0f;
    float centerY = getHeight() / 2.0f;
    
    // Select images based on hover state
    const juce::Image& img1 = isHovered ? vortexImage1Hover : vortexImage1;
    const juce::Image& img2 = isHovered ? vortexImage2Hover : vortexImage2;
    const juce::Image& img3 = isHovered ? vortexImage3Hover : vortexImage3;
    const juce::Image& img4 = isHovered ? vortexImage4Hover : vortexImage4;
    
    // Draw vortex layer 1 (bottom layer, slowest rotation)
    if (img1.isValid())
    {
        float scaleX = (float)getWidth() / img1.getWidth();
        float scaleY = (float)getHeight() / img1.getHeight();
        
        juce::AffineTransform transform = juce::AffineTransform::translation (-img1.getWidth() / 2.0f, -img1.getHeight() / 2.0f)
            .scaled (scaleX, scaleY)
            .rotated (rotation1)
            .translated (centerX, centerY);
        
        g.drawImageTransformed (img1, transform);
    }
    
    // Draw vortex layer 2 (medium-slow rotation)
    if (img2.isValid())
    {
        float scaleX = (float)getWidth() / img2.getWidth();
        float scaleY = (float)getHeight() / img2.getHeight();
        
        juce::AffineTransform transform = juce::AffineTransform::translation (-img2.getWidth() / 2.0f, -img2.getHeight() / 2.0f)
            .scaled (scaleX, scaleY)
            .rotated (rotation2)
            .translated (centerX, centerY);
        
        g.drawImageTransformed (img2, transform);
    }
    
    // Draw vortex layer 3 (medium-fast rotation)
    if (img3.isValid())
    {
        float scaleX = (float)getWidth() / img3.getWidth();
        float scaleY = (float)getHeight() / img3.getHeight();
        
        juce::AffineTransform transform = juce::AffineTransform::translation (-img3.getWidth() / 2.0f, -img3.getHeight() / 2.0f)
            .scaled (scaleX, scaleY)
            .rotated (rotation3)
            .translated (centerX, centerY);
        
        g.drawImageTransformed (img3, transform);
    }
    
    // Draw vortex layer 4 (top layer, fastest rotation)
    if (img4.isValid())
    {
        float scaleX = (float)getWidth() / img4.getWidth();
        float scaleY = (float)getHeight() / img4.getHeight();
        
        juce::AffineTransform transform = juce::AffineTransform::translation (-img4.getWidth() / 2.0f, -img4.getHeight() / 2.0f)
            .scaled (scaleX, scaleY)
            .rotated (rotation4)
            .translated (centerX, centerY);
        
        g.drawImageTransformed (img4, transform);
    }
}

void MassPoint::resized()
{
    // Layout child components here
}

//==============================================================================
void MassPoint::mouseDown (const juce::MouseEvent& event)
{
    LOG_INFO("MassPoint::mouseDown - Position in component: (" + 
             juce::String(event.position.x, 2) + ", " + juce::String(event.position.y, 2) + 
             "), Button: " + (event.mods.isLeftButtonDown() ? "Left" : 
                             event.mods.isRightButtonDown() ? "Right" : "Other") +
             ", Is popup menu: " + (event.mods.isPopupMenu() ? "Yes" : "No") +
             ", Current radius: " + juce::String(radius));
    
    // Right-click shows size menu
    if (event.mods.isPopupMenu())
    {
        LOG_INFO("MassPoint::mouseDown - Showing size menu");
        showSizeMenu();
        return;
    }
    
    // Start dragging (left-click)
    dragger.startDraggingComponent (this, event);
    LOG_INFO("Started dragging MassPoint from position (" + 
             juce::String(getX()) + ", " + juce::String(getY()) + ")");
}

void MassPoint::showSizeMenu()
{
    LOG_INFO("MassPoint::showSizeMenu - Opening size menu, current radius: " + juce::String(radius));
    
    juce::PopupMenu menu;
    menu.setLookAndFeel (&popupMenuLookAndFeel);
    
    menu.addItem (1, "small", true, radius == 50);
    menu.addItem (2, "medium", true, radius == 100);
    menu.addItem (3, "large", true, radius == 150);
    menu.addItem (4, "massive", true, radius == 200);
    menu.addSeparator();
    menu.addItem (5, "delete", true);
    
    menu.showMenuAsync (juce::PopupMenu::Options(),
                        [this] (int result)
                        {
                            LOG_INFO("MassPoint::showSizeMenu - Menu result: " + juce::String(result));
                            
                            if (result == 0)
                            {
                                LOG_INFO("MassPoint::showSizeMenu - User cancelled");
                                return; // User cancelled
                            }
                            
                            if (result == 5)
                            {
                                LOG_INFO("MassPoint - Requesting deletion");
                                // Delete request
                                if (onDeleteRequested)
                                    onDeleteRequested();
                                return;
                            }
                            
                            int newRadius = 50;
                            switch (result)
                            {
                                case 1: newRadius = 50; break;
                                case 2: newRadius = 100; break;
                                case 3: newRadius = 150; break;
                                case 4: newRadius = 200; break;
                            }
                            
                            LOG_INFO("MassPoint - Setting new radius: " + juce::String(newRadius) + 
                                     " (was: " + juce::String(radius) + ")");
                            setRadius (newRadius);
                        });
}

void MassPoint::setRadius (int newRadius)
{
    if (newRadius == radius)
        return;
    
    radius = newRadius;
    
    // Update mass multiplier based on radius (larger = stronger gravity)
    massMultiplier = static_cast<float>(radius) / static_cast<float>(minRadius);
    
    // Resize component (keep centered)
    auto oldBounds = getBounds();
    auto oldCenter = oldBounds.getCentre();
    setSize (radius, radius);
    setCentrePosition (oldCenter);
    
    LOG_INFO("MassPoint size changed to radius " + juce::String(radius) + 
             " with mass multiplier " + juce::String(massMultiplier));
    
    // Trigger repaint for gravity waves
    if (onMassMoved)
        onMassMoved();
    
    repaint();
}

void MassPoint::mouseDrag (const juce::MouseEvent& event)
{
    LOG_INFO("MassPoint::mouseDrag - Event position: (" + 
             juce::String(event.position.x, 2) + ", " + juce::String(event.position.y, 2) + ")");
    
    // Constrain dragging so that the CENTER stays within parent bounds
    // (Allow the mass point to extend beyond edges as long as center is inside)
    if (getParentComponent() != nullptr)
    {
        // Set minimum onscreen amounts to allow center to be at edges
        // This means the mass can extend beyond the bounds
        int halfWidth = getWidth() / 2;
        int halfHeight = getHeight() / 2;
        
        constrainer.setMinimumOnscreenAmounts (halfHeight, halfWidth, 
                                               halfHeight, halfWidth);
    }
    
    // Perform the drag
    dragger.dragComponent (this, event, &constrainer);
    
    LOG_INFO("MassPoint::mouseDrag - New position: (" + 
             juce::String(getX()) + ", " + juce::String(getY()) + ")");
    
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

void MassPoint::mouseEnter (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    LOG_INFO("MassPoint::mouseEnter - Mouse entered mass point (radius: " + 
             juce::String(radius) + ")");
    isHovered = true;
    repaint();
}

void MassPoint::mouseExit (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    LOG_INFO("MassPoint::mouseExit - Mouse exited mass point");
    isHovered = false;
    repaint();
}
