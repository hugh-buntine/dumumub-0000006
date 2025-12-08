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
    
private:
    juce::Image knobImage;
    juce::Image knobHoverImage;
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
    ToggleImageButton breakCpuButton;
    
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
    SliderWithTooltip lifespanSlider;
    juce::Label lifespanLabel;
    SliderWithTooltip masterGainSlider;
    juce::Label masterGainLabel;
    
    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainFreqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lifespanAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterGainAttachment;
    
    // Background images
    juce::Image backgroundImage;
    juce::Image canvasBackgroundImage;
    juce::Image canvasBorderImage;
    juce::Image titleImage;
    juce::Image sliderCasesImage;
    juce::Image dropTextImage;
    
    // Button images
    juce::Image graphicsButtonUnpressed;
    juce::Image graphicsButtonUnpressedHover;
    juce::Image graphicsButtonPressed;
    juce::Image graphicsButtonPressedHover;
    juce::Image breakCpuButtonUnpressed;
    juce::Image breakCpuButtonUnpressedHover;
    juce::Image breakCpuButtonPressed;
    juce::Image breakCpuButtonPressedHover;
    
    // Custom font
    juce::Typeface::Ptr customTypeface;
    
    // Custom LookAndFeel for each slider
    CustomSliderLookAndFeel attackLookAndFeel;
    CustomSliderLookAndFeel releaseLookAndFeel;
    CustomSliderLookAndFeel lifespanLookAndFeel;
    CustomSliderLookAndFeel grainSizeLookAndFeel;
    CustomSliderLookAndFeel grainFreqLookAndFeel;
    CustomSliderLookAndFeel masterGainLookAndFeel;
    
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
