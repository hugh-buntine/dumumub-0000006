#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);

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
    canvas.setBounds (50, 50, 400, 400);
    
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
    
    // Setup add spawn point button
    addAndMakeVisible (addSpawnPointButton);
    addSpawnPointButton.onClick = [this]() { canvas.newSpawnPoint(); };
    
    // Setup add mass point button
    addAndMakeVisible (addMassPointButton);
    addMassPointButton.onClick = [this]() { canvas.newMassPoint(); };
    
    // Setup emit particle button
    addAndMakeVisible (emitParticleButton);
    emitParticleButton.onClick = [this]() { canvas.spawnParticle(); };
}

PluginEditor::~PluginEditor()
{
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (255, 255, 242));

}

void PluginEditor::resized()
{
    // layout the positions of your child components here
    inspectButton.setBounds (getWidth() - 50, getHeight() - 50, 50, 50);
    
    // Audio file label at top
    audioFileLabel.setBounds (50, 10, 400, 30);
    
    addSpawnPointButton.setBounds (50, 700, 100, 50);
    addMassPointButton.setBounds (200, 700, 100, 50);
    emitParticleButton.setBounds (320, 700, 120, 50);
}
