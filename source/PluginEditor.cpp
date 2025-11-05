#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);
    
    // Load background images
    backgroundImage = juce::ImageCache::getFromMemory (BinaryData::BACKGROUND_png, 
                                                       BinaryData::BACKGROUND_pngSize);
    canvasBackgroundImage = juce::ImageCache::getFromMemory (BinaryData::CANVAS_png, 
                                                             BinaryData::CANVAS_pngSize);
    canvasBorderImage = juce::ImageCache::getFromMemory (BinaryData::CANVSBORDER_png, 
                                                         BinaryData::CANVSBORDER_pngSize);
    titleImage = juce::ImageCache::getFromMemory (BinaryData::TITLE_png, 
                                                  BinaryData::TITLE_pngSize);
    
    // Load and set star image for particles
    auto starImage = juce::ImageCache::getFromMemory (BinaryData::STAR_png, 
                                                      BinaryData::STAR_pngSize);
    Particle::setStarImage (starImage);

    addAndMakeVisible (inspectButton);

    // this chunk of code instantiates and opens the melatonin inspector
    inspectButton.onClick = [&] {
        if (!inspector)
        {
            inspector = std::make_unique<melatonin::Inspector> (*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }

        inspector->setVisible (true);
    };

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (500, 800);

    addAndMakeVisible (canvas);
    canvas.setBounds (50, 125, 400, 400);
    
    // Give processor reference to canvas for audio rendering
    processorRef.setCanvas (&canvas);
    
    // Setup audio file loading callback
    canvas.onAudioFileLoaded = [this](const juce::File& file) {
        processorRef.loadAudioFile (file);
        // Update label with filename
        audioFileLabel.setText ("Audio: " + file.getFileName(), juce::dontSendNotification);
        // Pass audio buffer to canvas for waveform display
        canvas.setAudioBuffer (processorRef.getAudioBuffer());
    };
    
    // Setup audio file label
    addAndMakeVisible (audioFileLabel);
    audioFileLabel.setFont (juce::FontOptions (14.0f));
    audioFileLabel.setJustificationType (juce::Justification::centred);
    audioFileLabel.setText ("No audio file loaded", juce::dontSendNotification);
    audioFileLabel.setColour (juce::Label::textColourId, juce::Colours::black);
    
    // Setup particle count label
    addAndMakeVisible (particleCountLabel);
    particleCountLabel.setFont (juce::FontOptions (14.0f));
    particleCountLabel.setJustificationType (juce::Justification::centredLeft);
    particleCountLabel.setText ("Particles: 0", juce::dontSendNotification);
    particleCountLabel.setColour (juce::Label::textColourId, juce::Colours::black);
    
    // Start timer to update particle count (10 Hz)
    startTimer (100);
    
    // Setup add spawn point button
    addAndMakeVisible (addSpawnPointButton);
    addSpawnPointButton.onClick = [this]() { canvas.newSpawnPoint(); };
    
    // Setup add mass point button
    addAndMakeVisible (addMassPointButton);
    addMassPointButton.onClick = [this]() { canvas.newMassPoint(); };
    
    // Setup emit particle button
    addAndMakeVisible (emitParticleButton);
    emitParticleButton.onClick = [this]() { canvas.spawnParticle(); };
    
    // Setup parameter sliders
    auto& apvts = processorRef.getAPVTS();
    
    // Grain Size
    addAndMakeVisible (grainSizeSlider);
    grainSizeSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    grainSizeSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    grainSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "grainSize", grainSizeSlider);
    addAndMakeVisible (grainSizeLabel);
    grainSizeLabel.setText ("Grain Size", juce::dontSendNotification);
    grainSizeLabel.setJustificationType (juce::Justification::centred);
    grainSizeLabel.setColour (juce::Label::textColourId, juce::Colours::black);
    
    // Grain Frequency
    addAndMakeVisible (grainFreqSlider);
    grainFreqSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    grainFreqSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    grainFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "grainFreq", grainFreqSlider);
    addAndMakeVisible (grainFreqLabel);
    grainFreqLabel.setText ("Frequency", juce::dontSendNotification);
    grainFreqLabel.setJustificationType (juce::Justification::centred);
    grainFreqLabel.setColour (juce::Label::textColourId, juce::Colours::black);
    
    // Attack
    addAndMakeVisible (attackSlider);
    attackSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    attackSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "attack", attackSlider);
    addAndMakeVisible (attackLabel);
    attackLabel.setText ("Attack", juce::dontSendNotification);
    attackLabel.setJustificationType (juce::Justification::centred);
    attackLabel.setColour (juce::Label::textColourId, juce::Colours::black);
    
    // Release
    addAndMakeVisible (releaseSlider);
    releaseSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    releaseSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "release", releaseSlider);
    addAndMakeVisible (releaseLabel);
    releaseLabel.setText ("Release", juce::dontSendNotification);
    releaseLabel.setJustificationType (juce::Justification::centred);
    releaseLabel.setColour (juce::Label::textColourId, juce::Colours::black);
    
    // Lifespan
    addAndMakeVisible (lifespanSlider);
    lifespanSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    lifespanSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    lifespanAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "lifespan", lifespanSlider);
    addAndMakeVisible (lifespanLabel);
    lifespanLabel.setText ("Lifespan", juce::dontSendNotification);
    lifespanLabel.setJustificationType (juce::Justification::centred);
    lifespanLabel.setColour (juce::Label::textColourId, juce::Colours::black);
    
    // Update canvas lifespan when parameter changes
    lifespanSlider.onValueChange = [this]() {
        canvas.setParticleLifespan (lifespanSlider.getValue());
    };
    
    // Master Gain
    addAndMakeVisible (masterGainSlider);
    masterGainSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    masterGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    masterGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "masterGain", masterGainSlider);
    addAndMakeVisible (masterGainLabel);
    masterGainLabel.setText ("Master Gain", juce::dontSendNotification);
    masterGainLabel.setJustificationType (juce::Justification::centred);
    masterGainLabel.setColour (juce::Label::textColourId, juce::Colours::black);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
}

void PluginEditor::timerCallback()
{
    // Update particle count display
    auto& lock = canvas.getParticlesLock();
    const juce::ScopedLock scopedLock (lock);
    auto* particles = canvas.getParticles();
    
    if (particles != nullptr)
    {
        int count = particles->size();
        particleCountLabel.setText ("Particles: " + juce::String(count), juce::dontSendNotification);
    }
}

void PluginEditor::paint (juce::Graphics& g)
{
    // Draw background image, scaled to fit the entire component
    if (backgroundImage.isValid())
    {
        g.drawImage (backgroundImage, getLocalBounds().toFloat(),
                    juce::RectanglePlacement::fillDestination);
    }
    else
    {
        // Fallback if image fails to load
        g.fillAll (juce::Colour (255, 255, 242));
    }
    
    // Draw canvas background where the canvas is (behind the canvas)
    if (canvasBackgroundImage.isValid())
    {
        // Draw at (25, 100) with 450x450 size
        g.drawImage (canvasBackgroundImage, juce::Rectangle<float> (25.0f, 100.0f, 450.0f, 450.0f),
                    juce::RectanglePlacement::fillDestination);
    }
    
    // Draw canvas border on top at (0, 70) with 500x500 size
    // This goes on top of everything including the canvas due to paint order
    if (canvasBorderImage.isValid())
    {
        g.drawImage (canvasBorderImage, juce::Rectangle<float> (0.0f, 70.0f, 500.0f, 500.0f),
                    juce::RectanglePlacement::fillDestination);
    }
    
    // Draw title at top (0, 0) with 500x118 size
    if (titleImage.isValid())
    {
        g.drawImage (titleImage, juce::Rectangle<float> (0.0f, 0.0f, 500.0f, 118.0f),
                    juce::RectanglePlacement::fillDestination);
    }
}

void PluginEditor::resized()
{
    // layout the positions of your child components here
    inspectButton.setBounds (getWidth() - 50, getHeight() - 50, 50, 50);
    
    // Audio file label at top
    audioFileLabel.setBounds (50, 10, 300, 30);
    
    // Particle count label at top right
    particleCountLabel.setBounds (360, 10, 90, 30);
    
    // Canvas - positioned at (50, 125) with 400x400 size
    canvas.setBounds (50, 125, 400, 400);
    
    // Buttons below canvas
    addSpawnPointButton.setBounds (50, 460, 100, 30);
    addMassPointButton.setBounds (170, 460, 100, 30);
    emitParticleButton.setBounds (290, 460, 120, 30);
    
    // Parameter knobs below buttons (two rows)
    int knobSize = 80;
    int knobY1 = 500;
    int knobY2 = 620;
    int knobSpacing = 100;
    int startX = 50;
    int labelHeight = 20;
    int labelOffset = -22;
    
    // Row 1: Grain Size, Frequency, Attack
    grainSizeLabel.setBounds (startX, knobY1 + labelOffset, knobSize, labelHeight);
    grainSizeSlider.setBounds (startX, knobY1, knobSize, knobSize);
    
    grainFreqLabel.setBounds (startX + knobSpacing, knobY1 + labelOffset, knobSize, labelHeight);
    grainFreqSlider.setBounds (startX + knobSpacing, knobY1, knobSize, knobSize);
    
    attackLabel.setBounds (startX + knobSpacing * 2, knobY1 + labelOffset, knobSize, labelHeight);
    attackSlider.setBounds (startX + knobSpacing * 2, knobY1, knobSize, knobSize);
    
    // Row 2: Release, Lifespan, Master Gain
    releaseLabel.setBounds (startX, knobY2 + labelOffset, knobSize, labelHeight);
    releaseSlider.setBounds (startX, knobY2, knobSize, knobSize);
    
    lifespanLabel.setBounds (startX + knobSpacing, knobY2 + labelOffset, knobSize, labelHeight);
    lifespanSlider.setBounds (startX + knobSpacing, knobY2, knobSize, knobSize);
    
    masterGainLabel.setBounds (startX + knobSpacing * 2, knobY2 + labelOffset, knobSize, labelHeight);
    masterGainSlider.setBounds (startX + knobSpacing * 2, knobY2, knobSize, knobSize);
}
