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
    setSize (radius, radius);
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    LOG_INFO("MassPoint created with radius " + juce::String(radius));
}

MassPoint::~MassPoint()
{
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
    float centerX = getWidth() / 2.0f;
    float centerY = getHeight() / 2.0f;
    
    const juce::Image& img1 = isHovered ? vortexImage1Hover : vortexImage1;
    const juce::Image& img2 = isHovered ? vortexImage2Hover : vortexImage2;
    const juce::Image& img3 = isHovered ? vortexImage3Hover : vortexImage3;
    const juce::Image& img4 = isHovered ? vortexImage4Hover : vortexImage4;
    
    // Layer 1 (bottom, slowest)
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
    
    // Layer 2
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
    
    // Layer 3
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
    
    // Layer 4 (top, fastest)
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
}

//==============================================================================
void MassPoint::mouseDown (const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu())
    {
        showSizeMenu();
        return;
    }
    
    dragger.startDraggingComponent (this, event);
    LOG_INFO("Started dragging MassPoint from (" + juce::String(getX()) + ", " + juce::String(getY()) + ")");
}

void MassPoint::showSizeMenu()
{
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
                            if (result == 0)
                                return;
                            
                            if (result == 5)
                            {
                                LOG_INFO("MassPoint - Requesting deletion");
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
                            
                            setRadius (newRadius);
                        });
}

void MassPoint::setRadius (int newRadius)
{
    if (newRadius == radius)
        return;
    
    radius = newRadius;
    massMultiplier = static_cast<float>(radius) / static_cast<float>(minRadius);
    
    auto oldBounds = getBounds();
    auto oldCenter = oldBounds.getCentre();
    setSize (radius, radius);
    setCentrePosition (oldCenter);
    
    LOG_INFO("MassPoint radius changed to " + juce::String(radius));
    
    if (onMassMoved)
        onMassMoved();
    
    repaint();
}

void MassPoint::mouseDrag (const juce::MouseEvent& event)
{
    if (getParentComponent() != nullptr)
    {
        int halfWidth = getWidth() / 2;
        int halfHeight = getHeight() / 2;
        constrainer.setMinimumOnscreenAmounts (halfHeight, halfWidth, 
                                               halfHeight, halfWidth);
    }
    
    dragger.dragComponent (this, event, &constrainer);
    
    if (onMassMoved)
        onMassMoved();
}

void MassPoint::mouseUp (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    LOG_INFO("Stopped dragging MassPoint at (" + juce::String(getX()) + ", " + juce::String(getY()) + ")");
    
    if (onMassDropped)
        onMassDropped();
}

void MassPoint::mouseEnter (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    isHovered = true;
    repaint();
}

void MassPoint::mouseExit (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    isHovered = false;
    repaint();
}
