#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

//==============================================================================
// Custom LookAndFeel for popup menus
class CustomPopupMenuLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomPopupMenuLookAndFeel()
    {
        int duruSansSize = 0;
        auto duruSansData = BinaryData::getNamedResource ("DuruSans_ttf", duruSansSize);
        if (duruSansData != nullptr && duruSansSize > 0)
            customTypeface = juce::Typeface::createSystemTypefaceFor (duruSansData, static_cast<size_t>(duruSansSize));
        
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff141400));
        setColour (juce::PopupMenu::textColourId, juce::Colour (0xffFFFFF2));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff2a2a00));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xffFFFFF2));
    }
    
    juce::Font getPopupMenuFont() override
    {
        if (customTypeface != nullptr)
            return juce::Font (juce::FontOptions (customTypeface).withHeight (14.0f));
        return juce::Font (juce::FontOptions (14.0f));
    }
    
    int getPopupMenuBorderSize() override
    {
        return 12;
    }
    
    void getIdealPopupMenuItemSize (const juce::String& text,
                                   bool isSeparator,
                                   int standardMenuItemHeight,
                                   int& idealWidth,
                                   int& idealHeight) override
    {
        if (isSeparator)
        {
            idealWidth = 50;
            idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight / 10 : 10;
        }
        else
        {
            auto font = getPopupMenuFont();
            
            if (standardMenuItemHeight > 0 && font.getHeight() > standardMenuItemHeight / 1.3f)
                font.setHeight (standardMenuItemHeight / 1.3f);
            
            idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight : juce::roundToInt (font.getHeight() * 1.3f);
            juce::GlyphArrangement glyphs;
            glyphs.addLineOfText (font, text, 0.0f, 0.0f);
            idealWidth = juce::roundToInt (glyphs.getBoundingBox (0, -1, true).getWidth()) + idealHeight * 3;
        }
    }
    
    void drawPopupMenuBackgroundWithOptions (juce::Graphics& g,
                                            int width,
                                            int height,
                                            const juce::PopupMenu::Options&) override
    {
        g.fillAll (juce::Colour (0xff141400));
        
        g.setColour (juce::Colour (0xffFFFFF2).withAlpha (0.3f));
        g.drawRoundedRectangle (0.0f, 0.0f, (float) width, (float) height, 12.0f, 1.0f);
    }
    
    void drawPopupMenuItem (juce::Graphics& g,
                           const juce::Rectangle<int>& area,
                           bool isSeparator,
                           bool isActive,
                           bool isHighlighted,
                           bool isTicked,
                           bool hasSubMenu,
                           const juce::String& text,
                           const juce::String& shortcutKeyText,
                           const juce::Drawable* icon,
                           const juce::Colour* textColourToUse) override
    {
        juce::ignoreUnused (icon, shortcutKeyText, isTicked, hasSubMenu);
        
        if (isSeparator)
        {
            auto r = area.reduced (5, 0);
            r.removeFromTop (r.getHeight() / 2 - 1);
            g.setColour (juce::Colour (0xffFFFFF2).withAlpha (0.3f));
            g.fillRect (r.removeFromTop (1));
        }
        else
        {
            auto textColour = juce::Colour (0xffFFFFF2);
            
            if (isHighlighted && isActive)
            {
                g.setColour (juce::Colour (0xff2a2a00).withAlpha (0.8f));
                g.fillRect (area);
            }
            
            if (!isActive)
                textColour = textColour.withAlpha (0.3f);
            
            if (textColourToUse != nullptr)
                textColour = *textColourToUse;
            
            g.setColour (textColour);
            
            auto r = area.reduced (25, 3);
            
            auto font = getPopupMenuFont();
            font.setExtraKerningFactor (0.15f);
            g.setFont (font);
            
            auto lowerText = text.toLowerCase();
            g.drawFittedText (lowerText, r, juce::Justification::centred, 1);
        }
    }
    
private:
    juce::Typeface::Ptr customTypeface;
};
