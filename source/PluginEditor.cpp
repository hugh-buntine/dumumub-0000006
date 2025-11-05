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
    sliderCasesImage = juce::ImageCache::getFromMemory (BinaryData::SLIDERCASES_png, 
                                                        BinaryData::SLIDERCASES_pngSize);
    
    // Load knob images and set up custom LookAndFeel for each slider
    auto knob1 = juce::ImageCache::getFromMemory (BinaryData::KNOB1_png, BinaryData::KNOB1_pngSize);
    auto knob1Hover = juce::ImageCache::getFromMemory (BinaryData::KNOB1HOVER_png, BinaryData::KNOB1HOVER_pngSize);
    attackLookAndFeel.setKnobImages (knob1, knob1Hover);
    
    auto knob2 = juce::ImageCache::getFromMemory (BinaryData::KNOB2_png, BinaryData::KNOB2_pngSize);
    auto knob2Hover = juce::ImageCache::getFromMemory (BinaryData::KNOB2HOVER_png, BinaryData::KNOB2HOVER_pngSize);
    releaseLookAndFeel.setKnobImages (knob2, knob2Hover);
    
    auto knob3 = juce::ImageCache::getFromMemory (BinaryData::KNOB3_png, BinaryData::KNOB3_pngSize);
    auto knob3Hover = juce::ImageCache::getFromMemory (BinaryData::KNOB3HOVER_png, BinaryData::KNOB3HOVER_pngSize);
    lifespanLookAndFeel.setKnobImages (knob3, knob3Hover);
    
    auto knob4 = juce::ImageCache::getFromMemory (BinaryData::KNOB4_png, BinaryData::KNOB4_pngSize);
    auto knob4Hover = juce::ImageCache::getFromMemory (BinaryData::KNOB4HOVER_png, BinaryData::KNOB4HOVER_pngSize);
    grainSizeLookAndFeel.setKnobImages (knob4, knob4Hover);
    
    auto knob5 = juce::ImageCache::getFromMemory (BinaryData::KNOB5_png, BinaryData::KNOB5_pngSize);
    auto knob5Hover = juce::ImageCache::getFromMemory (BinaryData::KNOB5HOVER_png, BinaryData::KNOB5HOVER_pngSize);
    grainFreqLookAndFeel.setKnobImages (knob5, knob5Hover);
    
    auto knob6 = juce::ImageCache::getFromMemory (BinaryData::KNOB6_png, BinaryData::KNOB6_pngSize);
    auto knob6Hover = juce::ImageCache::getFromMemory (BinaryData::KNOB6HOVER_png, BinaryData::KNOB6HOVER_pngSize);
    masterGainLookAndFeel.setKnobImages (knob6, knob6Hover);
    
    // Load and set star image for particles
    auto starImage = juce::ImageCache::getFromMemory (BinaryData::STAR_png, 
                                                      BinaryData::STAR_pngSize);
    Particle::setStarImage (starImage);
    
    // Load and set spawner images for spawn points
    auto spawnerImage1 = juce::ImageCache::getFromMemory (BinaryData::SPAWNER1_png, 
                                                          BinaryData::SPAWNER1_pngSize);
    auto spawnerImage2 = juce::ImageCache::getFromMemory (BinaryData::SPAWNER2_png, 
                                                          BinaryData::SPAWNER2_pngSize);
    SpawnPoint::setSpawnerImages (spawnerImage1, spawnerImage2);
    
    // Load and set hover spawner images for spawn points
    auto spawnerImage1Hover = juce::ImageCache::getFromMemory (BinaryData::SPAWNER1HOVER_png, 
                                                               BinaryData::SPAWNER1HOVER_pngSize);
    auto spawnerImage2Hover = juce::ImageCache::getFromMemory (BinaryData::SPAWNER2HOVER_png, 
                                                               BinaryData::SPAWNER2HOVER_pngSize);
    SpawnPoint::setSpawnerHoverImages (spawnerImage1Hover, spawnerImage2Hover);
    
    // Load and set vortex images for mass points (black holes)
    auto vortexImage1 = juce::ImageCache::getFromMemory (BinaryData::VORTEX1_png, 
                                                         BinaryData::VORTEX1_pngSize);
    auto vortexImage2 = juce::ImageCache::getFromMemory (BinaryData::VORTEX2_png, 
                                                         BinaryData::VORTEX2_pngSize);
    auto vortexImage3 = juce::ImageCache::getFromMemory (BinaryData::VORTEX3_png, 
                                                         BinaryData::VORTEX3_pngSize);
    auto vortexImage4 = juce::ImageCache::getFromMemory (BinaryData::VORTEX4_png, 
                                                         BinaryData::VORTEX4_pngSize);
    MassPoint::setVortexImages (vortexImage1, vortexImage2, vortexImage3, vortexImage4);
    
    // Load and set hover vortex images for mass points
    auto vortexImage1Hover = juce::ImageCache::getFromMemory (BinaryData::VORTEX1HOVER_png, 
                                                              BinaryData::VORTEX1HOVER_pngSize);
    auto vortexImage2Hover = juce::ImageCache::getFromMemory (BinaryData::VORTEX2HOVER_png, 
                                                              BinaryData::VORTEX2HOVER_pngSize);
    auto vortexImage3Hover = juce::ImageCache::getFromMemory (BinaryData::VORTEX3HOVER_png, 
                                                              BinaryData::VORTEX3HOVER_pngSize);
    auto vortexImage4Hover = juce::ImageCache::getFromMemory (BinaryData::VORTEX4HOVER_png, 
                                                              BinaryData::VORTEX4HOVER_pngSize);
    MassPoint::setVortexHoverImages (vortexImage1Hover, vortexImage2Hover, vortexImage3Hover, vortexImage4Hover);

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
    
    // Attack
    addAndMakeVisible (attackSlider);
    attackSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    attackSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    attackSlider.setLookAndFeel (&attackLookAndFeel);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "attack", attackSlider);
    
    // Release
    addAndMakeVisible (releaseSlider);
    releaseSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    releaseSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    releaseSlider.setLookAndFeel (&releaseLookAndFeel);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "release", releaseSlider);
    
    // Lifespan
    addAndMakeVisible (lifespanSlider);
    lifespanSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    lifespanSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    lifespanSlider.setLookAndFeel (&lifespanLookAndFeel);
    lifespanAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "lifespan", lifespanSlider);
    
    // Update canvas lifespan when parameter changes
    lifespanSlider.onValueChange = [this]() {
        canvas.setParticleLifespan (lifespanSlider.getValue());
    };
    
    // Grain Size
    addAndMakeVisible (grainSizeSlider);
    grainSizeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    grainSizeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    grainSizeSlider.setLookAndFeel (&grainSizeLookAndFeel);
    grainSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "grainSize", grainSizeSlider);
    
    // Grain Frequency
    addAndMakeVisible (grainFreqSlider);
    grainFreqSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    grainFreqSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    grainFreqSlider.setLookAndFeel (&grainFreqLookAndFeel);
    grainFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "grainFreq", grainFreqSlider);
    
    // Master Gain
    addAndMakeVisible (masterGainSlider);
    masterGainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    masterGainSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    masterGainSlider.setLookAndFeel (&masterGainLookAndFeel);
    masterGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "masterGain", masterGainSlider);
}

PluginEditor::~PluginEditor()
{
    // Reset LookAndFeel before sliders are destroyed
    attackSlider.setLookAndFeel (nullptr);
    releaseSlider.setLookAndFeel (nullptr);
    lifespanSlider.setLookAndFeel (nullptr);
    grainSizeSlider.setLookAndFeel (nullptr);
    grainFreqSlider.setLookAndFeel (nullptr);
    masterGainSlider.setLookAndFeel (nullptr);
    
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
    
    // Draw slider cases at (40, 560) with 415x185 size
    if (sliderCasesImage.isValid())
    {
        g.drawImage (sliderCasesImage, juce::Rectangle<float> (40.0f, 560.0f, 415.0f, 185.0f),
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
    
    // Horizontal sliders in slider cases area (40, 560, 415x185)
    // 2 columns, 3 rows, each slider 200x50
    // Slider cases dimensions: x=40, y=560, width=415, height=185
    int sliderCasesX = 40;
    int sliderCasesY = 560;
    int sliderWidth = 200;
    int sliderHeight = 50;
    int columnSpacing = 215; // Space between left edges of columns
    int rowSpacing = 60;     // Space between rows
    int leftColumnX = sliderCasesX + 5;  // Small margin from left edge
    int rightColumnX = leftColumnX + columnSpacing - 10; // Move right column 10px left (was 15, now +5)
    int startY = sliderCasesY + 10; // Small margin from top
    
    // Row 1: Attack (left), Release (right) - moved up 10px, then down 3px = -7px
    attackSlider.setBounds (leftColumnX, startY - 7, sliderWidth, sliderHeight);
    releaseSlider.setBounds (rightColumnX, startY - 7, sliderWidth, sliderHeight);
    
    // Row 2: Lifespan (left), Grain Size (right) - moved up 1px
    lifespanSlider.setBounds (leftColumnX, startY + rowSpacing - 1, sliderWidth, sliderHeight);
    grainSizeSlider.setBounds (rightColumnX, startY + rowSpacing - 1, sliderWidth, sliderHeight);
    
    // Row 3: Frequency (left), Master Gain (right) - moved down 5px, then up 2px = +3px
    grainFreqSlider.setBounds (leftColumnX, startY + rowSpacing * 2 + 3, sliderWidth, sliderHeight);
    masterGainSlider.setBounds (rightColumnX, startY + rowSpacing * 2 + 3, sliderWidth, sliderHeight);
}
