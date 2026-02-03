#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);
    
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
    sliderCasesCoverImage = juce::ImageCache::getFromMemory (BinaryData::SLIDERCASESCOVER_png, 
                                                              BinaryData::SLIDERCASESCOVER_pngSize);
    dropTextImage = juce::ImageCache::getFromMemory (BinaryData::DROPTEXT_png, 
                                                     BinaryData::DROPTEXT_pngSize);
    
    customTypeface = juce::Typeface::createSystemTypefaceFor (BinaryData::DuruSansRegular_ttf,
                                                               BinaryData::DuruSansRegular_ttfSize);
    
    if (customTypeface != nullptr)
        DBG("Duru Sans font loaded successfully. Family: " + customTypeface->getName());
    else
        DBG("ERROR: Duru Sans font failed to load!");
    
    auto knob1 = juce::ImageCache::getFromMemory (BinaryData::KNOB1_png, BinaryData::KNOB1_pngSize);
    auto knob1Hover = juce::ImageCache::getFromMemory (BinaryData::KNOB1HOVER_png, BinaryData::KNOB1HOVER_pngSize);
    attackLookAndFeel.setKnobImages (knob1, knob1Hover);
    
    auto knob2 = juce::ImageCache::getFromMemory (BinaryData::KNOB2_png, BinaryData::KNOB2_pngSize);
    auto knob2Hover = juce::ImageCache::getFromMemory (BinaryData::KNOB2HOVER_png, BinaryData::KNOB2HOVER_pngSize);
    releaseLookAndFeel.setKnobImages (knob2, knob2Hover);
    
    auto knob3 = juce::ImageCache::getFromMemory (BinaryData::KNOB3_png, BinaryData::KNOB3_pngSize);
    auto knob3Hover = juce::ImageCache::getFromMemory (BinaryData::KNOB3HOVER_png, BinaryData::KNOB3HOVER_pngSize);
    decayLookAndFeel.setKnobImages (knob3, knob3Hover);
    
    auto knob4 = juce::ImageCache::getFromMemory (BinaryData::KNOB4_png, BinaryData::KNOB4_pngSize);
    auto knob4Hover = juce::ImageCache::getFromMemory (BinaryData::KNOB4HOVER_png, BinaryData::KNOB4HOVER_pngSize);
    sustainLookAndFeel.setKnobImages (knob4, knob4Hover);
    
    auto knob5 = juce::ImageCache::getFromMemory (BinaryData::KNOB5_png, BinaryData::KNOB5_pngSize);
    auto knob5Hover = juce::ImageCache::getFromMemory (BinaryData::KNOB5HOVER_png, BinaryData::KNOB5HOVER_pngSize);
    grainFreqLookAndFeel.setKnobImages (knob5, knob5Hover);
    
    auto knob6 = juce::ImageCache::getFromMemory (BinaryData::KNOB6_png, BinaryData::KNOB6_pngSize);
    auto knob6Hover = juce::ImageCache::getFromMemory (BinaryData::KNOB6HOVER_png, BinaryData::KNOB6HOVER_pngSize);
    grainSizeLookAndFeel.setKnobImages (knob6, knob6Hover);
    
    auto gainKnob = juce::ImageCache::getFromMemory (BinaryData::GAINKNOB_png, BinaryData::GAINKNOB_pngSize);
    auto gainKnobHover = juce::ImageCache::getFromMemory (BinaryData::GAINKNOBHOVER_png, BinaryData::GAINKNOBHOVER_pngSize);
    masterGainLookAndFeel.setKnobImages (gainKnob, gainKnobHover);
    
    graphicsButtonUnpressed = juce::ImageCache::getFromMemory (BinaryData::GRAPHICSBUTTONUNPRESSED_png, 
                                                               BinaryData::GRAPHICSBUTTONUNPRESSED_pngSize);
    graphicsButtonUnpressedHover = juce::ImageCache::getFromMemory (BinaryData::GRAPHICSBUTTONUNPRESSEDHOVER_png, 
                                                                    BinaryData::GRAPHICSBUTTONUNPRESSEDHOVER_pngSize);
    graphicsButtonPressed = juce::ImageCache::getFromMemory (BinaryData::GRAPHICSBUTTONPRESSED_png, 
                                                             BinaryData::GRAPHICSBUTTONPRESSED_pngSize);
    graphicsButtonPressedHover = juce::ImageCache::getFromMemory (BinaryData::GRAPHICSBUTTONPRESSEDHOVER_png, 
                                                                  BinaryData::GRAPHICSBUTTONPRESSEDHOVER_pngSize);
    
    addAndMakeVisible (graphicsButton);
    graphicsButton.setImages (graphicsButtonUnpressed, graphicsButtonUnpressedHover,
                             graphicsButtonPressed, graphicsButtonPressedHover);
    graphicsButton.onClick = [this]() {
        LOG_INFO("Graphics button clicked: " + juce::String(graphicsButton.getToggleState() ? "ON" : "OFF"));
        canvas.setBounceMode (graphicsButton.getToggleState());
    };
    
    auto starImage = juce::ImageCache::getFromMemory (BinaryData::STAR_png, 
                                                      BinaryData::STAR_pngSize);
    Particle::setStarImage (starImage);
    
    auto spawnerImage1 = juce::ImageCache::getFromMemory (BinaryData::SPAWNER1_png, 
                                                          BinaryData::SPAWNER1_pngSize);
    auto spawnerImage2 = juce::ImageCache::getFromMemory (BinaryData::SPAWNER2_png, 
                                                          BinaryData::SPAWNER2_pngSize);
    SpawnPoint::setSpawnerImages (spawnerImage1, spawnerImage2);
    
    auto spawnerImage1Hover = juce::ImageCache::getFromMemory (BinaryData::SPAWNER1HOVER_png, 
                                                               BinaryData::SPAWNER1HOVER_pngSize);
    auto spawnerImage2Hover = juce::ImageCache::getFromMemory (BinaryData::SPAWNER2HOVER_png, 
                                                               BinaryData::SPAWNER2HOVER_pngSize);
    SpawnPoint::setSpawnerHoverImages (spawnerImage1Hover, spawnerImage2Hover);
    
    auto vortexImage1 = juce::ImageCache::getFromMemory (BinaryData::VORTEX1_png, 
                                                         BinaryData::VORTEX1_pngSize);
    auto vortexImage2 = juce::ImageCache::getFromMemory (BinaryData::VORTEX2_png, 
                                                         BinaryData::VORTEX2_pngSize);
    auto vortexImage3 = juce::ImageCache::getFromMemory (BinaryData::VORTEX3_png, 
                                                         BinaryData::VORTEX3_pngSize);
    auto vortexImage4 = juce::ImageCache::getFromMemory (BinaryData::VORTEX4_png, 
                                                         BinaryData::VORTEX4_pngSize);
    MassPoint::setVortexImages (vortexImage1, vortexImage2, vortexImage3, vortexImage4);
    
    auto vortexImage1Hover = juce::ImageCache::getFromMemory (BinaryData::VORTEX1HOVER_png, 
                                                              BinaryData::VORTEX1HOVER_pngSize);
    auto vortexImage2Hover = juce::ImageCache::getFromMemory (BinaryData::VORTEX2HOVER_png, 
                                                              BinaryData::VORTEX2HOVER_pngSize);
    auto vortexImage3Hover = juce::ImageCache::getFromMemory (BinaryData::VORTEX3HOVER_png, 
                                                              BinaryData::VORTEX3HOVER_pngSize);
    auto vortexImage4Hover = juce::ImageCache::getFromMemory (BinaryData::VORTEX4HOVER_png, 
                                                              BinaryData::VORTEX4HOVER_pngSize);
    MassPoint::setVortexHoverImages (vortexImage1Hover, vortexImage2Hover, vortexImage3Hover, vortexImage4Hover);

    canvasBorderComponent = std::make_unique<ImageComponent>();
    canvasBorderComponent->setImage(canvasBorderImage);
    addAndMakeVisible(*canvasBorderComponent);
    canvasBorderComponent->setBounds(0, 70, 500, 500);
    canvasBorderComponent->setAlwaysOnTop(true);
    
    sliderCasesComponent = std::make_unique<ImageComponent>();
    sliderCasesComponent->setImage(sliderCasesImage);
    addAndMakeVisible(*sliderCasesComponent);
    sliderCasesComponent->setBounds(40, 560, 415, 185);
    sliderCasesComponent->setAlwaysOnTop(true);
    
    sliderCasesCoverComponent = std::make_unique<ImageComponent>();
    sliderCasesCoverComponent->setImage(sliderCasesCoverImage);
    addAndMakeVisible(*sliderCasesCoverComponent);
    sliderCasesCoverComponent->setBounds(10, 562, 480, 182);
    sliderCasesCoverComponent->setAlwaysOnTop(true);
    
    titleComponent = std::make_unique<ImageComponent>();
    titleComponent->setImage(titleImage);
    addAndMakeVisible(*titleComponent);
    titleComponent->setBounds(0, 0, 500, 118);
    titleComponent->setAlwaysOnTop(true);

    setSize (500, 800);

    addAndMakeVisible (canvas);
    canvas.setBounds (50, 125, 400, 400);
    
    processorRef.setCanvas (&canvas);
    canvas.setCustomTypeface (customTypeface);
    
    canvas.onAudioFileLoaded = [this](const juce::File& file) {
        processorRef.loadAudioFile (file);
        audioFileLabel.setText (file.getFileName(), juce::dontSendNotification);
        canvas.setAudioBuffer (processorRef.getAudioBuffer());
    };
    
    addAndMakeVisible (audioFileLabel);
    if (customTypeface != nullptr)
        audioFileLabel.setFont (juce::FontOptions (customTypeface).withHeight (12.0f));
    else
        audioFileLabel.setFont (juce::FontOptions (12.0f));
    audioFileLabel.setJustificationType (juce::Justification::centred);
    audioFileLabel.setText ("", juce::dontSendNotification);
    audioFileLabel.setColour (juce::Label::textColourId, juce::Colours::transparentBlack);
    audioFileLabel.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    
    addAndMakeVisible (particleCountLabel);
    particleCountLabel.setFont (juce::FontOptions (14.0f));
    particleCountLabel.setJustificationType (juce::Justification::centredLeft);
    particleCountLabel.setText ("0", juce::dontSendNotification);
    particleCountLabel.setColour (juce::Label::textColourId, juce::Colours::transparentBlack);
    particleCountLabel.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    
    startTimer (100);
    
    auto& apvts = processorRef.getAPVTS();
    
    addAndMakeVisible (attackSlider);
    attackSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    attackSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    attackSlider.setLookAndFeel (&attackLookAndFeel);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "attack", attackSlider);
    attackSlider.onValueChange = [this]() { attackSlider.repaint(); };
    attackSlider.onDragStateChanged = [this](bool isDragging, double value) {
        showingSliderValue = isDragging;
        showingADSRCurve = isDragging;
        activeSliderName = "ATTACK";
        activeSliderValue = value;
        repaint();
    };
    
    addAndMakeVisible (releaseSlider);
    releaseSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    releaseSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    releaseSlider.setLookAndFeel (&releaseLookAndFeel);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "release", releaseSlider);
    releaseSlider.onValueChange = [this]() { releaseSlider.repaint(); };
    releaseSlider.onDragStateChanged = [this](bool isDragging, double value) {
        showingSliderValue = isDragging;
        showingADSRCurve = isDragging;
        activeSliderName = "RELEASE";
        activeSliderValue = value;
        repaint();
    };
    
    addAndMakeVisible (decaySlider);
    decaySlider.setSliderStyle (juce::Slider::LinearHorizontal);
    decaySlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    decaySlider.setLookAndFeel (&decayLookAndFeel);
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "decay", decaySlider);
    decaySlider.onValueChange = [this]() { decaySlider.repaint(); };
    decaySlider.onDragStateChanged = [this](bool isDragging, double value) {
        showingSliderValue = isDragging;
        showingADSRCurve = isDragging;
        activeSliderName = "DECAY";
        activeSliderValue = value;
        repaint();
    };
    
    addAndMakeVisible (sustainSlider);
    sustainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    sustainSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    sustainSlider.setLookAndFeel (&sustainLookAndFeel);
    sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "sustain", sustainSlider);
    
    sustainSlider.onValueChange = [this]() {
        sustainSlider.repaint();
    };
    sustainSlider.onDragStateChanged = [this](bool isDragging, double value) {
        showingSliderValue = isDragging;
        showingADSRCurve = isDragging;
        activeSliderName = "SUSTAIN";
        activeSliderValue = value * 100.0;
        repaint();
    };
    
    addAndMakeVisible (grainSizeSlider);
    grainSizeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    grainSizeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    grainSizeSlider.setLookAndFeel (&grainSizeLookAndFeel);
    grainSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "grainSize", grainSizeSlider);
    grainSizeSlider.onValueChange = [this]() { grainSizeSlider.repaint(); };
    grainSizeSlider.onDragStateChanged = [this](bool isDragging, double value) {
        showingSliderValue = isDragging;
        showingGrainSizeWaveform = isDragging;
        activeSliderName = "GRAIN SIZE";
        activeSliderValue = value;
        repaint();
    };
    
    addAndMakeVisible (grainFreqSlider);
    grainFreqSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    grainFreqSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    grainFreqSlider.setLookAndFeel (&grainFreqLookAndFeel);
    grainFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "grainFreq", grainFreqSlider);
    grainFreqSlider.onValueChange = [this]() { grainFreqSlider.repaint(); };
    grainFreqSlider.onDragStateChanged = [this](bool isDragging, double value) {
        showingSliderValue = isDragging;
        showingGrainFreqWaveforms = isDragging;
        activeSliderName = "GRAIN FREQ";
        activeSliderValue = value;
        repaint();
    };
    
    addAndMakeVisible (masterGainSlider);
    masterGainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    masterGainSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    masterGainSlider.setLookAndFeel (&masterGainLookAndFeel);
    masterGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "masterGain", masterGainSlider);
    masterGainSlider.onValueChange = [this]() { masterGainSlider.repaint(); };
    masterGainSlider.onDragStateChanged = [this](bool isDragging, double value) {
        showingSliderValue = isDragging;
        showingGainVisualization = isDragging;
        activeSliderName = "MASTER GAIN";
        activeSliderValue = value;
        repaint();
    };
    
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
    attackSlider.setLookAndFeel (nullptr);
    releaseSlider.setLookAndFeel (nullptr);
    decaySlider.setLookAndFeel (nullptr);
    sustainSlider.setLookAndFeel (nullptr);
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
    if (backgroundImage.isValid())
    {
        g.drawImage (backgroundImage, getLocalBounds().toFloat(),
                    juce::RectanglePlacement::fillDestination);
    }
    else
    {
        g.fillAll (juce::Colour (255, 255, 242));
    }
    
    if (canvasBackgroundImage.isValid())
    {
        g.drawImage (canvasBackgroundImage, juce::Rectangle<float> (25.0f, 100.0f, 450.0f, 450.0f),
                    juce::RectanglePlacement::fillDestination);
    }
    
    if (showingSliderValue && customTypeface != nullptr)
    {
        juce::String valueText;
        if (activeSliderValue >= 100.0)
            valueText = juce::String (static_cast<int>(activeSliderValue));
        else if (activeSliderValue >= 10.0)
            valueText = juce::String (activeSliderValue, 1);
        else
            valueText = juce::String (activeSliderValue, 2);
        
        g.setColour (juce::Colour (0xFF, 0xFF, 0xF2).withAlpha (0.4f));
        auto font = juce::Font (juce::FontOptions (customTypeface).withHeight (80.0f));
        g.setFont (font);
        
        auto canvasBounds = canvas.getBounds();
        auto centerX = canvasBounds.getCentreX();
        auto centerY = canvasBounds.getCentreY();
        
        g.drawText (valueText, 
                   juce::Rectangle<float>(centerX - 200.0f, centerY - 40.0f, 400.0f, 80.0f), 
                   juce::Justification::centred, true);
    }
    
    if (showingADSRCurve)
        drawADSRCurve (g);
    
    if (showingGrainSizeWaveform)
        drawGrainSizeWaveform (g);
    
    if (showingGrainFreqWaveforms)
        drawGrainSizeWaveform (g);
    
    if (showingGainVisualization)
        drawGainVisualization (g);
}

void PluginEditor::paintOverChildren (juce::Graphics& g)
{
    auto labelBounds = audioFileLabel.getBounds();
    
    if (audioFileLabel.getText().isEmpty())
    {
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
        g.setColour (juce::Colour (0xFF, 0xFF, 0xF2).withAlpha (0.25f));
        
        if (customTypeface != nullptr)
        {
            auto text = audioFileLabel.getText();
            if (text.contains("."))
                text = text.upToFirstOccurrenceOf(".", false, false);
            text = text.toLowerCase();
            
            auto font = juce::Font (juce::FontOptions (customTypeface).withHeight (12.0f));
            juce::GlyphArrangement glyphs;
            glyphs.addLineOfText (font, text, 0.0f, 0.0f);
            auto naturalWidth = glyphs.getBoundingBox (0, -1, true).getWidth();
            auto targetWidth = 340.0f;
            auto extraSpaceNeeded = targetWidth - naturalWidth;
            auto letterSpacing = (text.length() > 1) ? extraSpaceNeeded / (text.length() - 1) : 0.0f;
            
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
    
    if (customTypeface != nullptr)
    {
        auto text = particleCountLabel.getText();
        g.setColour (juce::Colour (0xFF, 0xFF, 0xF2).withAlpha (0.25f));
        auto font = juce::Font (juce::FontOptions (customTypeface).withHeight (16.0f));
        g.setFont (font);
        
        auto canvasBounds = canvas.getBounds();
        juce::GlyphArrangement textGlyphs;
        textGlyphs.addLineOfText (font, text, 0.0f, 0.0f);
        auto textWidth = textGlyphs.getBoundingBox (0, -1, true).getWidth();
        auto textX = canvasBounds.getRight() - textWidth - 20.0f;
        auto textY = canvasBounds.getBottom() - 40.0f;
        
        g.drawText (text, juce::Rectangle<float>(textX, textY, textWidth, 20.0f), 
                   juce::Justification::centredRight, true);
    }
}

void PluginEditor::drawADSRCurve (juce::Graphics& g)
{
    auto attack = attackSlider.getValue();
    auto decay = decaySlider.getValue();
    auto sustainLinear = sustainSlider.getValue();
    auto release = releaseSlider.getValue();
    
    // Linear sustain for visual - audio uses logarithmic conversion
    float sustain = static_cast<float>(sustainLinear);
    
    auto canvasBounds = canvas.getBounds();
    float curveX = canvasBounds.getX();
    float curveY = canvasBounds.getY() + (canvasBounds.getHeight() / 3.0f);
    float curveWidth = canvasBounds.getWidth();
    float curveHeight = canvasBounds.getHeight() * (2.0f / 3.0f);
    
    float totalTime = static_cast<float>(attack + decay + 0.5f + release);
    float timeScale = curveWidth / totalTime;
    
    juce::Path adsrPath;
    
    float startX = curveX;
    float baseY = curveY + curveHeight;
    adsrPath.startNewSubPath (startX, baseY);
    
    // Attack phase (exponential)
    int attackSteps = 20;
    for (int i = 1; i <= attackSteps; ++i)
    {
        float t = i / static_cast<float>(attackSteps);
        float normalized = t * t;
        float x = startX + (t * static_cast<float>(attack) * timeScale);
        float y = baseY - (normalized * curveHeight);
        adsrPath.lineTo (x, y);
    }
    
    // Decay phase (inverse exponential to sustain)
    float decayStartX = startX + (static_cast<float>(attack) * timeScale);
    int decaySteps = 15;
    for (int i = 1; i <= decaySteps; ++i)
    {
        float t = i / static_cast<float>(decaySteps);
        float invExp = 1.0f - std::pow (1.0f - t, 3.0f);
        float level = 1.0f - (invExp * (1.0f - sustain));
        float x = decayStartX + (t * static_cast<float>(decay) * timeScale);
        float y = baseY - (level * curveHeight);
        adsrPath.lineTo (x, y);
    }
    
    // Sustain phase (flat)
    float sustainStartX = decayStartX + (static_cast<float>(decay) * timeScale);
    float sustainEndX = sustainStartX + (0.5f * timeScale);
    float sustainY = baseY - (sustain * curveHeight);
    adsrPath.lineTo (sustainEndX, sustainY);
    
    // Release phase (inverse exponential to silence)
    int releaseSteps = 20;
    for (int i = 1; i <= releaseSteps; ++i)
    {
        float t = i / static_cast<float>(releaseSteps);
        float invExp = 1.0f - std::pow (1.0f - t, 4.0f);
        float level = sustain - (invExp * sustain);
        float x = sustainEndX + (t * static_cast<float>(release) * timeScale);
        float y = baseY - (level * curveHeight);
        adsrPath.lineTo (x, y);
    }
    
    // Close path
    float endX = sustainEndX + (static_cast<float>(release) * timeScale);
    adsrPath.lineTo (endX, baseY);
    adsrPath.lineTo (startX, baseY);
    adsrPath.closeSubPath();
    
    auto colour = juce::Colour (0xFF, 0xFF, 0xF2);
    juce::ColourGradient gradient (colour.withAlpha(0.08f), curveX, curveY,
                                   colour.withAlpha(0.02f), curveX, baseY, false);
    g.setGradientFill (gradient);
    g.fillPath (adsrPath);
    
    g.setColour (colour.withAlpha (0.15f));
    g.strokePath (adsrPath, juce::PathStrokeType (1.5f));
}

void PluginEditor::drawGrainSizeWaveform (juce::Graphics& g)
{
    const auto* audioBuffer = processorRef.getAudioBuffer();
    if (audioBuffer == nullptr || audioBuffer->getNumSamples() == 0)
        return;
    
    auto canvasBounds = canvas.getBounds();
    float canvasWidth = canvasBounds.getWidth();
    float canvasHeight = canvasBounds.getHeight();
    float canvasX = canvasBounds.getX();
    float canvasY = canvasBounds.getY();
    
    auto colour = juce::Colour (0xFF, 0xFF, 0xF2);
    
    if (showingGrainFreqWaveforms)
    {
        float frequency = static_cast<float>(grainFreqSlider.getValue());
        
        int wholeCircles = static_cast<int>(frequency);
        float fractionalPart = frequency - wholeCircles;
        
        float circleRadius = 5.0f;
        
        // Predetermined circle positions (normalized 0-1, relative to canvas)
        // These are hardcoded to look random but consistent
        static const float circlePositions[][2] = {
            {0.52f, 0.48f}, {0.38f, 0.62f}, {0.71f, 0.35f}, {0.29f, 0.41f}, {0.64f, 0.69f},
            {0.45f, 0.27f}, {0.82f, 0.58f}, {0.19f, 0.73f}, {0.56f, 0.15f}, {0.33f, 0.85f},
            {0.77f, 0.44f}, {0.41f, 0.56f}, {0.68f, 0.22f}, {0.24f, 0.67f}, {0.59f, 0.81f},
            {0.88f, 0.39f}, {0.15f, 0.49f}, {0.49f, 0.92f}, {0.73f, 0.13f}, {0.35f, 0.28f},
            {0.62f, 0.76f}, {0.27f, 0.54f}, {0.81f, 0.66f}, {0.43f, 0.19f}, {0.69f, 0.87f},
            {0.21f, 0.36f}, {0.58f, 0.61f}, {0.91f, 0.25f}, {0.37f, 0.78f}, {0.76f, 0.51f},
            {0.48f, 0.34f}, {0.84f, 0.72f}, {0.26f, 0.45f}, {0.65f, 0.18f}, {0.39f, 0.89f},
            {0.72f, 0.57f}, {0.18f, 0.64f}, {0.54f, 0.31f}, {0.87f, 0.83f}, {0.31f, 0.24f},
            {0.67f, 0.46f}, {0.44f, 0.74f}, {0.79f, 0.29f}, {0.23f, 0.59f}, {0.61f, 0.91f},
            {0.36f, 0.16f}, {0.74f, 0.68f}, {0.28f, 0.52f}, {0.85f, 0.37f}, {0.47f, 0.79f},
            {0.53f, 0.42f}, {0.92f, 0.63f}, {0.34f, 0.21f}, {0.71f, 0.86f}, {0.25f, 0.47f},
            {0.63f, 0.33f}, {0.46f, 0.71f}, {0.83f, 0.54f}, {0.32f, 0.88f}, {0.69f, 0.26f},
            {0.51f, 0.65f}, {0.89f, 0.48f}, {0.38f, 0.32f}, {0.76f, 0.77f}, {0.22f, 0.43f},
            {0.58f, 0.19f}, {0.42f, 0.84f}, {0.78f, 0.61f}, {0.29f, 0.38f}, {0.66f, 0.53f},
            {0.17f, 0.69f}, {0.55f, 0.23f}, {0.86f, 0.75f}, {0.41f, 0.51f}, {0.73f, 0.36f},
            {0.33f, 0.82f}, {0.64f, 0.14f}, {0.48f, 0.67f}, {0.81f, 0.45f}, {0.27f, 0.58f},
            {0.57f, 0.28f}, {0.93f, 0.71f}, {0.39f, 0.44f}, {0.75f, 0.89f}, {0.24f, 0.33f},
            {0.62f, 0.56f}, {0.45f, 0.18f}, {0.84f, 0.64f}, {0.35f, 0.79f}, {0.68f, 0.41f},
            {0.21f, 0.55f}, {0.59f, 0.93f}, {0.88f, 0.31f}, {0.43f, 0.72f}, {0.77f, 0.24f},
            {0.31f, 0.61f}, {0.65f, 0.47f}, {0.49f, 0.85f}, {0.82f, 0.38f}, {0.37f, 0.69f},
            {0.71f, 0.52f}, {0.26f, 0.27f}, {0.54f, 0.76f}, {0.91f, 0.59f}, {0.44f, 0.35f},
            {0.79f, 0.81f}, {0.32f, 0.46f}, {0.67f, 0.21f}, {0.47f, 0.88f}, {0.85f, 0.57f},
            {0.38f, 0.39f}, {0.74f, 0.73f}, {0.28f, 0.63f}, {0.61f, 0.17f}, {0.51f, 0.84f},
            {0.89f, 0.42f}, {0.36f, 0.68f}, {0.72f, 0.29f}, {0.23f, 0.54f}, {0.58f, 0.91f},
            {0.42f, 0.26f}, {0.76f, 0.65f}, {0.33f, 0.48f}, {0.69f, 0.83f}, {0.25f, 0.37f},
            {0.63f, 0.59f}, {0.46f, 0.22f}, {0.81f, 0.74f}, {0.37f, 0.51f}, {0.73f, 0.16f},
            {0.29f, 0.86f}, {0.66f, 0.43f}, {0.52f, 0.71f}, {0.87f, 0.34f}, {0.41f, 0.62f},
            {0.78f, 0.49f}, {0.34f, 0.77f}, {0.68f, 0.28f}, {0.24f, 0.56f}, {0.59f, 0.19f},
            {0.48f, 0.82f}, {0.83f, 0.47f}, {0.39f, 0.66f}, {0.75f, 0.31f}, {0.31f, 0.72f},
            {0.64f, 0.53f}, {0.21f, 0.41f}, {0.57f, 0.87f}, {0.92f, 0.38f}, {0.43f, 0.64f},
            {0.77f, 0.23f}, {0.35f, 0.58f}, {0.69f, 0.46f}, {0.27f, 0.81f}, {0.61f, 0.35f},
            {0.47f, 0.69f}, {0.84f, 0.52f}, {0.38f, 0.25f}, {0.72f, 0.78f}, {0.26f, 0.44f},
            {0.58f, 0.16f}, {0.91f, 0.67f}, {0.44f, 0.53f}, {0.79f, 0.36f}, {0.33f, 0.75f},
            {0.65f, 0.27f}, {0.49f, 0.89f}, {0.82f, 0.58f}, {0.36f, 0.42f}, {0.71f, 0.63f},
            {0.23f, 0.31f}, {0.56f, 0.79f}, {0.88f, 0.46f}, {0.42f, 0.68f}, {0.76f, 0.21f},
            {0.32f, 0.57f}, {0.67f, 0.84f}, {0.45f, 0.39f}, {0.81f, 0.72f}, {0.37f, 0.29f},
            {0.73f, 0.61f}, {0.28f, 0.48f}, {0.62f, 0.18f}, {0.51f, 0.86f}, {0.87f, 0.43f},
            {0.39f, 0.74f}, {0.75f, 0.32f}, {0.31f, 0.66f}, {0.64f, 0.49f}, {0.22f, 0.83f},
            {0.57f, 0.37f}, {0.93f, 0.69f}, {0.46f, 0.54f}, {0.79f, 0.24f}, {0.34f, 0.59f}
        };
        
        const int maxCircles = 200; // Maximum number of circles
        
        // Draw whole circles
        for (int i = 0; i < wholeCircles && i < maxCircles; ++i)
        {
            float x = canvasX + circlePositions[i][0] * canvasWidth;
            float y = canvasY + circlePositions[i][1] * canvasHeight;
            
            g.setColour (colour.withAlpha (0.15f));
            g.fillEllipse (x - circleRadius, y - circleRadius, circleRadius * 2, circleRadius * 2);
        }
        
        // Draw fractional circle if needed
        if (fractionalPart > 0.01f && wholeCircles < maxCircles)
        {
            float x = canvasX + circlePositions[wholeCircles][0] * canvasWidth;
            float y = canvasY + circlePositions[wholeCircles][1] * canvasHeight;
            
            float alpha = 0.15f * fractionalPart; // Scale alpha by fractional amount
            g.setColour (colour.withAlpha (alpha));
            g.fillEllipse (x - circleRadius, y - circleRadius, circleRadius * 2, circleRadius * 2);
        }
    }
    else
    {
        // Single waveform for grain size visualization
        // Get grain size in milliseconds and convert to samples
        auto grainSizeMs = grainSizeSlider.getValue();
        auto sampleRate = processorRef.getSampleRate();
        if (sampleRate <= 0.0)
            sampleRate = 44100.0; // Fallback
        
        int grainSizeSamples = static_cast<int>((grainSizeMs / 1000.0) * sampleRate);
        grainSizeSamples = juce::jlimit (1, audioBuffer->getNumSamples(), grainSizeSamples);
        
        // Sample from the middle of the audio buffer for the grain size duration
        int startSample = (audioBuffer->getNumSamples() - grainSizeSamples) / 2;
        startSample = juce::jlimit (0, audioBuffer->getNumSamples() - grainSizeSamples, startSample);
        
        juce::Path waveformPath;
        bool pathStarted = false;
        
        int numPoints = 200; // Number of points to sample for smooth curve
        float xStep = canvasWidth / static_cast<float>(numPoints);
        float waveformCenterY = canvasY + (canvasHeight * 0.5f);
        
        for (int i = 0; i < numPoints; ++i)
        {
            // Map i to sample index within the grain size
            float t = i / static_cast<float>(numPoints - 1);
            int sampleIndex = startSample + static_cast<int>(t * grainSizeSamples);
            sampleIndex = juce::jlimit (0, audioBuffer->getNumSamples() - 1, sampleIndex);
            
            // Get average magnitude across all channels
            float magnitude = 0.0f;
            for (int channel = 0; channel < audioBuffer->getNumChannels(); ++channel)
            {
                magnitude += audioBuffer->getSample (channel, sampleIndex);
            }
            magnitude /= audioBuffer->getNumChannels();
            
            // Scale magnitude to canvas height
            float scaleFactor = 0.3f;
            float x = canvasX + (i * xStep);
            float y = waveformCenterY - (magnitude * canvasHeight * scaleFactor);
            
            if (!pathStarted)
            {
                waveformPath.startNewSubPath (x, y);
                pathStarted = true;
            }
            else
            {
                waveformPath.lineTo (x, y);
            }
        }
        
        // Draw outline
        g.setColour (colour.withAlpha (0.15f));
        g.strokePath (waveformPath, juce::PathStrokeType (1.5f));
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
    
    int sliderCasesX = 40;
    int sliderCasesY = 560;
    int sliderWidth = 200;
    int sliderHeight = 50;
    int columnSpacing = 215;
    int rowSpacing = 60;
    int leftColumnX = sliderCasesX + 5;
    int rightColumnX = leftColumnX + columnSpacing - 10;
    int startY = sliderCasesY + 10;
    
    attackSlider.setBounds (leftColumnX, startY - 7, sliderWidth, sliderHeight);
    releaseSlider.setBounds (rightColumnX, startY - 7, sliderWidth, sliderHeight);
    
    decaySlider.setBounds (leftColumnX, startY + rowSpacing - 1, sliderWidth, sliderHeight);
    sustainSlider.setBounds (rightColumnX, startY + rowSpacing - 1, sliderWidth, sliderHeight);
    
    grainFreqSlider.setBounds (leftColumnX, startY + rowSpacing * 2 + 3, sliderWidth, sliderHeight);
    grainSizeSlider.setBounds (rightColumnX, startY + rowSpacing * 2 + 3, sliderWidth, sliderHeight);
    
    masterGainSlider.setBounds (247, 749, 234, sliderHeight);
    
    int buttonY = 750;
    int buttonWidth = 200;
    int buttonHeight = 40;
    int buttonSpacing = 15;
    
    int totalWidth = buttonWidth * 2 + buttonSpacing;
    int startX = sliderCasesX + (415 - totalWidth) / 2;
    
    graphicsButton.setBounds (startX, buttonY, buttonWidth, buttonHeight);
}

void PluginEditor::drawGainVisualization (juce::Graphics& g)
{
    auto gainDB = static_cast<float>(masterGainSlider.getValue());
    
    // Map -60dB (representing -∞) to 0.0, +6dB to 1.0
    float linearScale;
    if (gainDB <= -60.0f)
        linearScale = 0.0f;
    else
        linearScale = (gainDB + 60.0f) / 66.0f;
    
    auto canvasBounds = canvas.getBounds();
    float drawX = canvasBounds.getX();
    float drawY = canvasBounds.getY() + (canvasBounds.getHeight() / 3.0f);
    float drawWidth = canvasBounds.getWidth();
    float drawHeight = canvasBounds.getHeight() * (2.0f / 3.0f);
    
    if (linearScale > 0.0f)
    {
        float rectHeight = drawHeight * linearScale;
        float rectY = drawY + drawHeight - rectHeight;
        
        auto colour = juce::Colour (0xFF, 0xFF, 0xF2);
        juce::ColourGradient gradient (colour.withAlpha(0.08f), drawX, rectY,
                                       colour.withAlpha(0.02f), drawX, rectY + rectHeight, false);
        g.setGradientFill (gradient);
        g.fillRect (drawX, rectY, drawWidth, rectHeight);
        
        g.setColour (colour.withAlpha (0.15f));
        g.drawRect (drawX, rectY, drawWidth, rectHeight, 1.5f);
    }
}
