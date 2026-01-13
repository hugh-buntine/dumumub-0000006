#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"
#include "Canvas.h"
#include "Particle.h"
#include "SpawnPoint.h"
#include "MassPoint.h"
#include "Logger.h"

//==============================================================================
// Custom toggle button with 4 image states
class ToggleImageButton : public juce::Button
{
public:
    ToggleImageButton() : juce::Button ("") 
    {
        setClickingTogglesState (true);
    }
    
    void setImages (const juce::Image& normalOff, const juce::Image& hoverOff,
                   const juce::Image& normalOn, const juce::Image& hoverOn)
    {
        imageNormalOff = normalOff;
        imageHoverOff = hoverOff;
        imageNormalOn = normalOn;
        imageHoverOn = hoverOn;
        repaint();
    }
    
    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, 
                     bool shouldDrawButtonAsDown) override
    {
        juce::ignoreUnused (shouldDrawButtonAsDown);
        
        juce::Image imageToDraw;
        
        if (getToggleState())
        {
            // Button is pressed/on
            imageToDraw = shouldDrawButtonAsHighlighted ? imageHoverOn : imageNormalOn;
        }
        else
        {
            // Button is unpressed/off
            imageToDraw = shouldDrawButtonAsHighlighted ? imageHoverOff : imageNormalOff;
        }
        
        if (imageToDraw.isValid())
        {
            g.drawImage (imageToDraw, getLocalBounds().toFloat(),
                        juce::RectanglePlacement::fillDestination);
        }
    }
    
private:
    juce::Image imageNormalOff;
    juce::Image imageHoverOff;
    juce::Image imageNormalOn;
    juce::Image imageHoverOn;
};

//==============================================================================
// Custom LookAndFeel for sliders with knob images
class CustomSliderLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomSliderLookAndFeel() = default;
    
    void setKnobImages (const juce::Image& normal, const juce::Image& hover)
    {
        knobImage = normal;
        knobHoverImage = hover;
    }
    
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        juce::ignoreUnused (minSliderPos, maxSliderPos, style, x, width);
        
        // Draw the knob image (no track line)
        auto& imageToUse = slider.isMouseOverOrDragging() ? knobHoverImage : knobImage;
        if (imageToUse.isValid())
        {
            auto knobWidth = 40.0f;
            auto knobHeight = 40.0f;
            auto knobX = sliderPos - knobWidth * 0.5f;
            auto knobY = y + (height - knobHeight) * 0.5f;
            
            g.drawImage (imageToUse, juce::Rectangle<float> (knobX, knobY, knobWidth, knobHeight),
                        juce::RectanglePlacement::fillDestination);
        }
    }
    
protected:
    juce::Image knobImage;
    juce::Image knobHoverImage;
};

//==============================================================================
// Custom LookAndFeel for gain slider with scaling knob
class GainSliderLookAndFeel : public CustomSliderLookAndFeel
{
public:
    GainSliderLookAndFeel() = default;
    
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style != juce::Slider::LinearHorizontal)
        {
            CustomSliderLookAndFeel::drawLinearSlider (g, x, y, width, height, sliderPos, 
                                                        minSliderPos, maxSliderPos, style, slider);
            return;
        }
        
        juce::ignoreUnused (minSliderPos, maxSliderPos);
        
        // No track line for gain slider
        
        // Add padding to keep knob away from edges
        float sidePadding = 10.0f; // 10px padding on each side
        float usableWidth = width - (sidePadding * 2.0f);
        float usableX = x + sidePadding;
        
        // Calculate knob size based on slider position (not value)
        // Map sliderPos to the usable range with padding
        float normalizedPosition = (sliderPos - x) / width;
        normalizedPosition = juce::jlimit (0.0f, 1.0f, normalizedPosition);
        
        // Scale knob size: 20x20 at left (0.0), 40x40 at right (1.0)
        float minKnobSize = 20.0f;
        float maxKnobSize = 40.0f;
        float knobSize = minKnobSize + (normalizedPosition * (maxKnobSize - minKnobSize));
        
        // Position knob within the padded area
        float paddedSliderPos = usableX + (normalizedPosition * usableWidth);
        float trackY = y + height * 0.5f;
        float knobX = paddedSliderPos - knobSize * 0.5f;
        float knobY = trackY - knobSize * 0.5f;
        
        // Draw knob with scaled size
        juce::Image& imageToUse = slider.isMouseOverOrDragging() ? knobHoverImage : knobImage;
        if (imageToUse.isValid())
        {
            g.drawImage (imageToUse, juce::Rectangle<float> (knobX, knobY, knobSize, knobSize),
                        juce::RectanglePlacement::fillDestination);
        }
        else
        {
            // Fallback: draw a white circle if image isn't loaded
            g.setColour (juce::Colours::white);
            g.fillEllipse (knobX, knobY, knobSize, knobSize);
        }
    }
};

//==============================================================================
// Custom Slider that notifies parent when dragging
class SliderWithTooltip : public juce::Slider
{
public:
    SliderWithTooltip() = default;
    
    std::function<void(bool, double)> onDragStateChanged;
    
    void mouseDown (const juce::MouseEvent& e) override
    {
        juce::Slider::mouseDown (e);
        if (onDragStateChanged)
            onDragStateChanged (true, getValue());
    }
    
    void mouseDrag (const juce::MouseEvent& e) override
    {
        juce::Slider::mouseDrag (e);
        if (onDragStateChanged)
            onDragStateChanged (true, getValue());
    }
    
    void mouseUp (const juce::MouseEvent& e) override
    {
        juce::Slider::mouseUp (e);
        if (onDragStateChanged)
            onDragStateChanged (false, getValue());
    }
};

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    
    void paintOverChildren (juce::Graphics& g) override;
    void drawADSRCurve (juce::Graphics& g);
    void drawGrainSizeWaveform (juce::Graphics& g);

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PluginProcessor& processorRef;

    Canvas canvas { processorRef };
    
    // Image buttons
    ToggleImageButton graphicsButton;
    
    juce::Label audioFileLabel;
    juce::Label particleCountLabel;
    
    // Parameter controls
    SliderWithTooltip grainSizeSlider;
    juce::Label grainSizeLabel;
    SliderWithTooltip grainFreqSlider;
    juce::Label grainFreqLabel;
    SliderWithTooltip attackSlider;
    juce::Label attackLabel;
    SliderWithTooltip releaseSlider;
    juce::Label releaseLabel;
    SliderWithTooltip decaySlider;
    juce::Label decayLabel;
    SliderWithTooltip sustainSlider;
    juce::Label sustainLabel;
    SliderWithTooltip masterGainSlider;
    juce::Label masterGainLabel;
    
    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainFreqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterGainAttachment;
    
    // Background images
    juce::Image backgroundImage;
    juce::Image canvasBackgroundImage;
    juce::Image canvasBorderImage;
    juce::Image titleImage;
    juce::Image sliderCasesImage;
    juce::Image sliderCasesCoverImage;
    juce::Image dropTextImage;
    
    // Button images
    juce::Image graphicsButtonUnpressed;
    juce::Image graphicsButtonUnpressedHover;
    juce::Image graphicsButtonPressed;
    juce::Image graphicsButtonPressedHover;
    
    // Custom font
    juce::Typeface::Ptr customTypeface;
    
    // Custom LookAndFeel for each slider
    CustomSliderLookAndFeel attackLookAndFeel;
    CustomSliderLookAndFeel releaseLookAndFeel;
    CustomSliderLookAndFeel decayLookAndFeel;
    CustomSliderLookAndFeel sustainLookAndFeel;
    CustomSliderLookAndFeel grainSizeLookAndFeel;
    CustomSliderLookAndFeel grainFreqLookAndFeel;
    GainSliderLookAndFeel masterGainLookAndFeel;
    
    // Slider value display tracking
    bool showingSliderValue = false;
    juce::String activeSliderName;
    double activeSliderValue = 0.0;
    
    // ADSR curve display tracking
    bool showingADSRCurve = false;
    
    // Grain size waveform display tracking
    bool showingGrainSizeWaveform = false;
    
    // Grain frequency waveform display tracking
    bool showingGrainFreqWaveforms = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
