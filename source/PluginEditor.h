#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"
#include "Canvas.h"
#include "Particle.h"
#include "SpawnPoint.h"
#include "MassPoint.h"
#include "Logger.h"

//==============================================================================
// Simple image component for displaying images as components
class ImageComponent : public juce::Component
{
public:
    ImageComponent() = default;
    
    void setImage(const juce::Image& image)
    {
        img = image;
        repaint();
    }
    
    void paint(juce::Graphics& g) override
    {
        if (img.isValid())
        {
            g.drawImage(img, getLocalBounds().toFloat(), 
                       juce::RectanglePlacement::stretchToFit);
        }
    }
    
    // Make component click-through - mouse events pass to components underneath
    bool hitTest(int x, int y) override
    {
        juce::ignoreUnused(x, y);
        return false; // Never consume mouse events
    }
    
private:
    juce::Image img;
};

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
        
        // Draw the knob image (no track line) with rotation
        auto& imageToUse = slider.isMouseOverOrDragging() ? knobHoverImage : knobImage;
        if (imageToUse.isValid())
        {
            auto knobWidth = 40.0f;
            auto knobHeight = 40.0f;
            auto knobX = sliderPos - knobWidth * 0.5f;
            auto knobY = y + (height - knobHeight) * 0.5f;
            
            // Calculate rotation based on slider value (0.0 to 1.0)
            // Rotate from -135° to +135° (270° total range like a typical rotary knob)
            auto value = slider.getValue();
            auto range = slider.getRange();
            auto normalizedValue = (value - range.getStart()) / (range.getEnd() - range.getStart());
            float rotationRadians = -2.356f + (static_cast<float>(normalizedValue) * 4.712f); // -135° to +135° in radians
            
            // Draw rotated knob
            juce::Graphics::ScopedSaveState savedState (g);
            auto centerX = knobX + knobWidth * 0.5f;
            auto centerY = knobY + knobHeight * 0.5f;
            
            juce::AffineTransform transform = juce::AffineTransform::rotation (rotationRadians, centerX, centerY);
            g.addTransform (transform);
            
            g.drawImage (imageToUse, juce::Rectangle<float> (knobX, knobY, knobWidth, knobHeight),
                        juce::RectanglePlacement::fillDestination);
        }
    }
    
protected:
    juce::Image knobImage;
    juce::Image knobHoverImage;
};

//==============================================================================
// Custom LookAndFeel for gain slider with scaling knob and -infinity behavior
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
        
        // Custom position calculation for -infinity behavior
        // Reserve leftmost ~3 pixels for -infinity zone
        auto value = slider.getValue();
        auto range = slider.getRange();
        
        float normalizedPosition;
        if (value <= range.getStart()) // -60dB represents -infinity
        {
            normalizedPosition = 0.0f; // Leftmost position for -infinity
        }
        else
        {
            // Map -60dB to +6dB linearly, but start from a few pixels right of the left edge
            double normalizedValue = (value - range.getStart()) / (range.getEnd() - range.getStart());
            float minUsablePosition = 3.0f / width; // ~3 pixels from left edge
            normalizedPosition = minUsablePosition + (static_cast<float>(normalizedValue) * (1.0f - minUsablePosition));
        }
        
        // Scale knob size: 20x20 at left (0.0), 40x40 at right (1.0)
        float minKnobSize = 20.0f;
        float maxKnobSize = 40.0f;
        float knobSize = minKnobSize + (normalizedPosition * (maxKnobSize - minKnobSize));
        
        // Position knob within the padded area
        float paddedSliderPos = usableX + (normalizedPosition * usableWidth);
        float trackY = y + height * 0.5f;
        float knobX = paddedSliderPos - knobSize * 0.5f;
        float knobY = trackY - knobSize * 0.5f;
        
        // Calculate rotation based on slider value with -infinity consideration
        float normalizedValue;
        if (value <= range.getStart())
        {
            normalizedValue = 0.0f; // -infinity position (leftmost rotation)
        }
        else
        {
            normalizedValue = static_cast<float>((value - range.getStart()) / (range.getEnd() - range.getStart()));
        }
        
        float rotationRadians = -2.356f + (normalizedValue * 4.712f); // -135° to +135° in radians
        
        // Draw knob with scaled size and rotation
        juce::Image& imageToUse = slider.isMouseOverOrDragging() ? knobHoverImage : knobImage;
        if (imageToUse.isValid())
        {
            juce::Graphics::ScopedSaveState savedState (g);
            auto centerX = knobX + knobSize * 0.5f;
            auto centerY = knobY + knobSize * 0.5f;
            
            juce::AffineTransform transform = juce::AffineTransform::rotation (rotationRadians, centerX, centerY);
            g.addTransform (transform);
            
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
// Custom Gain Slider with -infinity behavior
class GainSlider : public juce::Slider
{
public:
    GainSlider() = default;
    
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
    
    double proportionOfLengthToValue (double proportion) override
    {
        auto range = getRange();
        
        // If at leftmost position (first ~3 pixels), return minimum value for -infinity display
        float minUsablePosition = 3.0f / getWidth();
        if (proportion <= minUsablePosition)
        {
            return range.getStart(); // -60dB which triggers -infinity display
        }
        
        // Map the rest of the slider range linearly
        double adjustedProportion = (proportion - minUsablePosition) / (1.0 - minUsablePosition);
        return range.getStart() + (adjustedProportion * range.getLength());
    }
    
    double valueToProportionOfLength (double value) override
    {
        auto range = getRange();
        
        if (value <= range.getStart()) // -60dB represents -infinity
        {
            return 0.0; // Leftmost position for -infinity
        }
        
        // Map -60dB to +6dB linearly, but start from a few pixels right of the left edge
        double normalizedValue = (value - range.getStart()) / range.getLength();
        float minUsablePosition = 3.0f / getWidth();
        return minUsablePosition + (normalizedValue * (1.0 - minUsablePosition));
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
    void drawGainVisualization (juce::Graphics& g);

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
    GainSlider masterGainSlider;
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
    
    // Image components for setAlwaysOnTop functionality
    std::unique_ptr<ImageComponent> canvasBorderComponent;
    std::unique_ptr<ImageComponent> sliderCasesComponent;
    std::unique_ptr<ImageComponent> sliderCasesCoverComponent;
    std::unique_ptr<ImageComponent> titleComponent;
    
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
    
    // Gain level visualization tracking
    bool showingGainVisualization = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
