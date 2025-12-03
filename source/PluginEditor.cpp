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
    dropTextImage = juce::ImageCache::getFromMemory (BinaryData::DROPTEXT_png, 
                                                     BinaryData::DROPTEXT_pngSize);
    
    // Load custom font (Duru Sans) - use the correct method for custom fonts from memory
    customTypeface = juce::Typeface::createSystemTypefaceFor (BinaryData::DuruSansRegular_ttf,
                                                               BinaryData::DuruSansRegular_ttfSize);
    
    // Debug: Check if font loaded
    if (customTypeface != nullptr)
        DBG("Duru Sans font loaded successfully. Family: " + customTypeface->getName());
    else
        DBG("ERROR: Duru Sans font failed to load!");
    
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
    
    // Load button images
    graphicsButtonUnpressed = juce::ImageCache::getFromMemory (BinaryData::GRAPHICSBUTTONUNPRESSED_png, 
                                                               BinaryData::GRAPHICSBUTTONUNPRESSED_pngSize);
    graphicsButtonUnpressedHover = juce::ImageCache::getFromMemory (BinaryData::GRAPHICSBUTTONUNPRESSEDHOVER_png, 
                                                                    BinaryData::GRAPHICSBUTTONUNPRESSEDHOVER_pngSize);
    graphicsButtonPressed = juce::ImageCache::getFromMemory (BinaryData::GRAPHICSBUTTONPRESSED_png, 
                                                             BinaryData::GRAPHICSBUTTONPRESSED_pngSize);
    graphicsButtonPressedHover = juce::ImageCache::getFromMemory (BinaryData::GRAPHICSBUTTONPRESSEDHOVER_png, 
                                                                  BinaryData::GRAPHICSBUTTONPRESSEDHOVER_pngSize);
    
    breakCpuButtonUnpressed = juce::ImageCache::getFromMemory (BinaryData::BREAKCPUUNPRESSED_png, 
                                                               BinaryData::BREAKCPUUNPRESSED_pngSize);
    breakCpuButtonUnpressedHover = juce::ImageCache::getFromMemory (BinaryData::BREAKCPUUNPRESSEDHOVER_png, 
                                                                    BinaryData::BREAKCPUUNPRESSEDHOVER_pngSize);
    breakCpuButtonPressed = juce::ImageCache::getFromMemory (BinaryData::BREAKCPUPRESSED_png, 
                                                             BinaryData::BREAKCPUPRESSED_pngSize);
    breakCpuButtonPressedHover = juce::ImageCache::getFromMemory (BinaryData::BREAKCPUPRESSEDHOVER_png, 
                                                                  BinaryData::BREAKCPUPRESSEDHOVER_pngSize);
    
    // Setup Graphics button
    addAndMakeVisible (graphicsButton);
    graphicsButton.setImages (graphicsButtonUnpressed, graphicsButtonUnpressedHover,
                             graphicsButtonPressed, graphicsButtonPressedHover);
    
    // Setup Break CPU button
    addAndMakeVisible (breakCpuButton);
    breakCpuButton.setImages (breakCpuButtonUnpressed, breakCpuButtonUnpressedHover,
                             breakCpuButtonPressed, breakCpuButtonPressedHover);
    breakCpuButton.onClick = [this]() {
        // Update canvas limits based on button state
        canvas.setBreakCpuMode (breakCpuButton.getToggleState());
    };
    
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
        audioFileLabel.setText (file.getFileName(), juce::dontSendNotification);
        // Pass audio buffer to canvas for waveform display
        canvas.setAudioBuffer (processorRef.getAudioBuffer());
    };
    
    // Setup audio file label (will be drawn manually in paintOverChildren)
    addAndMakeVisible (audioFileLabel);
    if (customTypeface != nullptr)
        audioFileLabel.setFont (juce::FontOptions (customTypeface).withHeight (12.0f));
    else
        audioFileLabel.setFont (juce::FontOptions (12.0f));
    audioFileLabel.setJustificationType (juce::Justification::centred);
    audioFileLabel.setText ("", juce::dontSendNotification); // Empty - we'll draw drop-text image or filename
    // Make label text transparent so we can draw it ourselves in paintOverChildren
    audioFileLabel.setColour (juce::Label::textColourId, juce::Colours::transparentBlack);
    audioFileLabel.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    
    // Setup particle count label (will be drawn manually in paintOverChildren)
    addAndMakeVisible (particleCountLabel);
    particleCountLabel.setFont (juce::FontOptions (14.0f));
    particleCountLabel.setJustificationType (juce::Justification::centredLeft);
    particleCountLabel.setText ("0", juce::dontSendNotification);
    // Make label text transparent so we can draw it ourselves in paintOverChildren
    particleCountLabel.setColour (juce::Label::textColourId, juce::Colours::transparentBlack);
    particleCountLabel.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    
    // Start timer to update particle count (10 Hz)
    startTimer (100);
    
    // Setup parameter sliders
    auto& apvts = processorRef.getAPVTS();
    
    // Attack
    addAndMakeVisible (attackSlider);
    attackSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    attackSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    attackSlider.setLookAndFeel (&attackLookAndFeel);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "attack", attackSlider);
    attackSlider.onValueChange = [this]() { attackSlider.repaint(); };
    attackSlider.onDragStateChanged = [this](bool isDragging, double value) {
        showingSliderValue = isDragging;
        activeSliderName = "ATTACK";
        activeSliderValue = value;
        repaint();
    };
    
    // Release
    addAndMakeVisible (releaseSlider);
    releaseSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    releaseSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    releaseSlider.setLookAndFeel (&releaseLookAndFeel);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "release", releaseSlider);
    releaseSlider.onValueChange = [this]() { releaseSlider.repaint(); };
    releaseSlider.onDragStateChanged = [this](bool isDragging, double value) {
        showingSliderValue = isDragging;
        activeSliderName = "RELEASE";
        activeSliderValue = value;
        repaint();
    };
    
    // Lifespan (now Sustain Level)
    addAndMakeVisible (lifespanSlider);
    lifespanSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    lifespanSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    lifespanSlider.setLookAndFeel (&lifespanLookAndFeel);
    lifespanAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "sustain", lifespanSlider);
    
    // Update display when parameter changes
    lifespanSlider.onValueChange = [this]() {
        lifespanSlider.repaint();
    };
    lifespanSlider.onDragStateChanged = [this](bool isDragging, double value) {
        showingSliderValue = isDragging;
        activeSliderName = "SUSTAIN";
        activeSliderValue = value * 100.0; // Display as percentage
        repaint();
    };
    
    // Grain Size
    addAndMakeVisible (grainSizeSlider);
    grainSizeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    grainSizeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    grainSizeSlider.setLookAndFeel (&grainSizeLookAndFeel);
    grainSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "grainSize", grainSizeSlider);
    grainSizeSlider.onValueChange = [this]() { grainSizeSlider.repaint(); };
    grainSizeSlider.onDragStateChanged = [this](bool isDragging, double value) {
        showingSliderValue = isDragging;
        activeSliderName = "GRAIN SIZE";
        activeSliderValue = value;
        repaint();
    };
    
    // Grain Frequency
    addAndMakeVisible (grainFreqSlider);
    grainFreqSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    grainFreqSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    grainFreqSlider.setLookAndFeel (&grainFreqLookAndFeel);
    grainFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "grainFreq", grainFreqSlider);
    grainFreqSlider.onValueChange = [this]() { grainFreqSlider.repaint(); };
    grainFreqSlider.onDragStateChanged = [this](bool isDragging, double value) {
        showingSliderValue = isDragging;
        activeSliderName = "GRAIN FREQ";
        activeSliderValue = value;
        repaint();
    };
    
    // Master Gain
    addAndMakeVisible (masterGainSlider);
    masterGainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    masterGainSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    masterGainSlider.setLookAndFeel (&masterGainLookAndFeel);
    masterGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "masterGain", masterGainSlider);
    masterGainSlider.onValueChange = [this]() { masterGainSlider.repaint(); };
    masterGainSlider.onDragStateChanged = [this](bool isDragging, double value) {
        showingSliderValue = isDragging;
        activeSliderName = "MASTER GAIN";
        activeSliderValue = value;
        repaint();
    };
    
    // Check if audio file was already loaded (from restored state)
    if (processorRef.hasAudioFileLoaded())
    {
        auto loadedFile = processorRef.getLoadedAudioFile();
        audioFileLabel.setText (loadedFile.getFileName(), juce::dontSendNotification);
        canvas.setAudioBuffer (processorRef.getAudioBuffer());
        LOG_INFO("Editor initialized with restored audio file: " + loadedFile.getFullPathName());
    }
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
        particleCountLabel.setText (juce::String(count), juce::dontSendNotification);
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
}

void PluginEditor::paintOverChildren (juce::Graphics& g)
{
    // Draw canvas border on top of everything at (0, 70) with 500x500 size
    if (canvasBorderImage.isValid())
    {
        g.drawImage (canvasBorderImage, juce::Rectangle<float> (0.0f, 70.0f, 500.0f, 500.0f),
                    juce::RectanglePlacement::fillDestination);
    }
    
    // Draw slider cases on top of sliders at (40, 560) with 415x185 size
    if (sliderCasesImage.isValid())
    {
        g.drawImage (sliderCasesImage, juce::Rectangle<float> (40.0f, 560.0f, 415.0f, 185.0f),
                    juce::RectanglePlacement::fillDestination);
    }
    
    // Draw title at top (0, 0) with 500x118 size - on top of everything
    if (titleImage.isValid())
    {
        g.drawImage (titleImage, juce::Rectangle<float> (0.0f, 0.0f, 500.0f, 118.0f),
                    juce::RectanglePlacement::fillDestination);
    }
    
    // Draw drop-text image or filename at top of canvas
    auto labelBounds = audioFileLabel.getBounds();
    
    if (audioFileLabel.getText().isEmpty())
    {
        // No file loaded - draw drop-text image at 360px wide
        if (dropTextImage.isValid())
        {
            auto imageWidth = 360.0f;
            auto imageHeight = 50.0f;
            auto x = labelBounds.getCentreX() - imageWidth * 0.5f;
            auto y = labelBounds.getY();
            
            g.drawImage (dropTextImage, juce::Rectangle<float> (x, y, imageWidth, imageHeight),
                        juce::RectanglePlacement::centred);
        }
    }
    else
    {
        // File loaded - draw filename in custom font with 25% opacity and letter spacing
        g.setColour (juce::Colour (0xFF, 0xFF, 0xF2).withAlpha (0.25f)); // #FFFFF2 at 25% opacity
        
        if (customTypeface != nullptr)
        {
            // Get text, remove extension, convert to lowercase
            auto text = audioFileLabel.getText();
            if (text.contains("."))
                text = text.upToFirstOccurrenceOf(".", false, false);
            text = text.toLowerCase();
            
            // Calculate letter spacing to spread text across 340px
            auto font = juce::Font (juce::FontOptions (customTypeface).withHeight (12.0f));
            auto naturalWidth = font.getStringWidthFloat (text);
            auto targetWidth = 340.0f;
            auto extraSpaceNeeded = targetWidth - naturalWidth;
            auto letterSpacing = (text.length() > 1) ? extraSpaceNeeded / (text.length() - 1) : 0.0f;
            
            // Draw text with custom letter spacing, moved down 12px
            font = font.withExtraKerningFactor (letterSpacing / font.getHeight());
            g.setFont (font);
            auto adjustedBounds = labelBounds.toFloat().withY(labelBounds.getY() + 12);
            g.drawText (text, adjustedBounds, juce::Justification::centred, true);
        }
        else
        {
            g.setFont (12.0f);
            auto adjustedBounds = labelBounds.toFloat().withY(labelBounds.getY() + 12);
            g.drawText (audioFileLabel.getText(), adjustedBounds, 
                       juce::Justification::centred, true);
        }
    }
    
    // Draw particle count in bottom right corner of canvas
    // Canvas is at (50, 125) with size 400x400, so bottom right is at (450, 525)
    if (customTypeface != nullptr)
    {
        auto text = particleCountLabel.getText(); // Just the number
        g.setColour (juce::Colour (0xFF, 0xFF, 0xF2).withAlpha (0.25f)); // #FFFFF2 at 25% opacity, same as filename
        auto font = juce::Font (juce::FontOptions (customTypeface).withHeight (16.0f)); // Bigger font (was 12.0f)
        g.setFont (font);
        
        // Position text in bottom right corner of canvas with more padding (further up and more left)
        auto canvasBounds = canvas.getBounds(); // Get canvas bounds
        auto textWidth = font.getStringWidthFloat (text);
        auto textX = canvasBounds.getRight() - textWidth - 20.0f; // 20px padding from right (was 10px)
        auto textY = canvasBounds.getBottom() - 40.0f; // 40px from bottom (higher than before)
        
        g.drawText (text, juce::Rectangle<float>(textX, textY, textWidth, 20.0f), 
                   juce::Justification::centredRight, true);
    }
    
    // Draw slider value in center of canvas when dragging
    if (showingSliderValue && customTypeface != nullptr)
    {
        // Format value based on range
        juce::String valueText;
        if (activeSliderValue >= 100.0)
            valueText = juce::String (static_cast<int>(activeSliderValue));
        else if (activeSliderValue >= 10.0)
            valueText = juce::String (activeSliderValue, 1);
        else
            valueText = juce::String (activeSliderValue, 2);
        
        g.setColour (juce::Colour (0xFF, 0xFF, 0xF2).withAlpha (0.4f)); // Slightly more opaque than particle count
        auto font = juce::Font (juce::FontOptions (customTypeface).withHeight (80.0f)); // Way bigger font
        g.setFont (font);
        
        // Draw in center of canvas
        auto canvasBounds = canvas.getBounds();
        auto centerX = canvasBounds.getCentreX();
        auto centerY = canvasBounds.getCentreY();
        
        g.drawText (valueText, 
                   juce::Rectangle<float>(centerX - 200.0f, centerY - 40.0f, 400.0f, 80.0f), 
                   juce::Justification::centred, true);
    }
}

void PluginEditor::resized()
{
    // Canvas - positioned at (50, 125) with 400x400 size
    canvas.setBounds (50, 125, 400, 400);
    
    // Audio file label at top of canvas (inside canvas area)
    audioFileLabel.setBounds (50, 130, 400, 25); // 5px from top of canvas
    
    // Particle count label (invisible, just holds the text) - positioned at bottom right of canvas
    particleCountLabel.setBounds (canvas.getRight() - 60, canvas.getBottom() - 25, 50, 20);
    
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
    
    // Image buttons below slider cases
    // Slider cases end at y=745 (560 + 185)
    int buttonY = 750; // 5px below slider cases
    int buttonWidth = 200;
    int buttonHeight = 40;
    int buttonSpacing = 15; // Space between buttons
    
    // Center both buttons horizontally in the slider cases area (40, 415 wide)
    int totalWidth = buttonWidth * 2 + buttonSpacing;
    int startX = sliderCasesX + (415 - totalWidth) / 2;
    
    graphicsButton.setBounds (startX, buttonY, buttonWidth, buttonHeight);
    breakCpuButton.setBounds (startX + buttonWidth + buttonSpacing, buttonY, buttonWidth, buttonHeight);
}
