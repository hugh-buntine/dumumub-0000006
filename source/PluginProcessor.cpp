#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Logger.h"
#include "Canvas.h"
#include <juce_audio_formats/juce_audio_formats.h>

//==============================================================================
PluginProcessor::PluginProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    // Initialize the logger
    Logger::getInstance().initialize("dumumub-0000006.log", "dumumub-0000006 Plugin Logger");
    LOG_INFO("PluginProcessor constructed");
}

PluginProcessor::~PluginProcessor()
{
    LOG_INFO("PluginProcessor destroyed");
    Logger::getInstance().shutdown();
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Grain Size (5ms - 500ms)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "grainSize",
        "Grain Size",
        juce::NormalisableRange<float> (5.0f, 500.0f, 1.0f, 0.5f),
        50.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String (value, 1) + " ms"; }
    ));
    
    // Grain Frequency (1 - 100 Hz)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "grainFreq",
        "Grain Frequency",
        juce::NormalisableRange<float> (1.0f, 100.0f, 0.1f, 0.4f),
        30.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String (value, 1) + " Hz"; }
    ));
    
    // Attack Time (0% - 100%)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "attack",
        "Attack",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        20.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String (value, 1) + " %"; }
    ));
    
    // Release Time (0% - 100%)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "release",
        "Release",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        20.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String (value, 1) + " %"; }
    ));
    
    // Lifespan (5s - 120s)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "lifespan",
        "Lifespan",
        juce::NormalisableRange<float> (5.0f, 120.0f, 1.0f, 0.5f),
        30.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String (value, 1) + " s"; }
    ));
    
    // Master Gain (-60dB - 0dB)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "masterGain",
        "Master Gain",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f),
        -6.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String (value, 1) + " dB"; }
    ));
    
    return layout;
}

//==============================================================================
const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused (sampleRate, samplesPerBlock);
    
    LOG_INFO("prepareToPlay called - Sample Rate: " + juce::String(sampleRate) + 
             " Hz, Buffer Size: " + juce::String(samplesPerBlock) + " samples");
}

void PluginProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear all output channels
    for (auto i = 0; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Process MIDI messages to spawn particles
    if (canvas != nullptr)
    {
        for (const auto metadata : midiMessages)
        {
            auto message = metadata.getMessage();
            
            if (message.isNoteOn())
            {
                int midiNote = message.getNoteNumber();
                float midiVelocity = message.getVelocity() / 127.0f; // Normalize to 0.0-1.0
                
                // Spawn particle from MIDI on the message thread (safe for canvas operations)
                juce::MessageManager::callAsync ([this, midiNote, midiVelocity]()
                {
                    if (canvas != nullptr)
                        canvas->spawnParticleFromMidi (midiNote, midiVelocity);
                });
            }
        }
    }

    // Check if we have audio file loaded and canvas available
    if (!hasAudioFileLoaded() || canvas == nullptr || audioFileBuffer.getNumSamples() == 0)
        return;
    
    // Get parameter values
    float grainSizeMs = apvts.getRawParameterValue("grainSize")->load();
    float grainFreq = apvts.getRawParameterValue("grainFreq")->load();
    float attackMs = apvts.getRawParameterValue("attack")->load();
    float releaseMs = apvts.getRawParameterValue("release")->load();
    float masterGainDb = apvts.getRawParameterValue("masterGain")->load();
    float masterGainLinear = juce::Decibels::decibelsToGain(masterGainDb);
    
    // Get particles from canvas (thread-safe)
    auto& particlesLock = canvas->getParticlesLock();
    const juce::ScopedLock lock (particlesLock);
    auto* particles = canvas->getParticles();
    
    if (particles == nullptr || particles->isEmpty())
        return;
    
    // Process each particle's grains
    for (auto* particle : *particles)
    {
        // Update sample rate if needed (cached internally)
        particle->updateSampleRate (getSampleRate());
        
        // Update grain parameters
        particle->setGrainParameters (grainSizeMs, attackMs, releaseMs);
        
        // Check if we should trigger a new grain based on frequency
        if (particle->shouldTriggerNewGrain (getSampleRate(), grainFreq))
        {
            particle->triggerNewGrain (audioFileBuffer.getNumSamples());
        }
        
        // Update all grains (advance playback, remove finished)
        particle->updateGrains (buffer.getNumSamples());
        
        // Get all active grains for this particle
        auto& grains = particle->getActiveGrains();
        
        // Process each active grain
        for (auto& grain : grains)
        {
            int grainStartSample = grain.startSample;
            int grainPosition = grain.playbackPosition;
            int totalGrainSamples = particle->getTotalGrainSamples();
            
            // Calculate how many samples to render this block
            int samplesToRender = juce::jmin (buffer.getNumSamples(), 
                                              totalGrainSamples - grainPosition);
            
            if (samplesToRender <= 0)
                continue;
            
            // Get edge crossfade info for smooth wraparound panning
            auto crossfade = particle->getEdgeCrossfade();
            float amplitude = particle->getGrainAmplitude (grain); // 0.0 to 1.0 (includes envelope)
            
            // Apply lifetime fade (quieter as particle dies)
            amplitude *= particle->getLifetimeAmplitude();
            
            // Apply MIDI velocity
            amplitude *= particle->getInitialVelocityMultiplier();
            
            // Apply master gain
            amplitude *= masterGainLinear;
            
            // Get pitch shift for this particle
            float pitchShift = particle->getPitchShift();
            
            // Calculate stereo gains from main pan
            float panAngle = (crossfade.mainPan + 1.0f) * juce::MathConstants<float>::pi / 4.0f;
            float leftGain = std::cos (panAngle) * amplitude;
            float rightGain = std::sin (panAngle) * amplitude;
            
            // If crossfading near edge, blend with opposite side
            float crossLeftGain = 0.0f;
            float crossRightGain = 0.0f;
            if (crossfade.crossfadeAmount > 0.0f)
            {
                float crossPanAngle = (crossfade.crossfadePan + 1.0f) * juce::MathConstants<float>::pi / 4.0f;
                crossLeftGain = std::cos (crossPanAngle) * amplitude * crossfade.crossfadeAmount;
                crossRightGain = std::sin (crossPanAngle) * amplitude * crossfade.crossfadeAmount;
                
                // Reduce main gains to maintain constant power
                float mainAmount = 1.0f - crossfade.crossfadeAmount;
                leftGain *= mainAmount;
                rightGain *= mainAmount;
            }
            
            // Render grain samples with pitch shift
            for (int i = 0; i < samplesToRender; ++i)
            {
                // Calculate source position with pitch shift (float for interpolation)
                float sourcePosition = grainStartSample + ((grainPosition + i) * pitchShift);
                
                // Linear interpolation between samples for smooth pitch shift
                int sourceSample1 = static_cast<int>(sourcePosition);
                int sourceSample2 = sourceSample1 + 1;
                float fraction = sourcePosition - sourceSample1;
                
                // Wrap samples if needed
                sourceSample1 = sourceSample1 % audioFileBuffer.getNumSamples();
                sourceSample2 = sourceSample2 % audioFileBuffer.getNumSamples();
                if (sourceSample1 < 0) sourceSample1 += audioFileBuffer.getNumSamples();
                if (sourceSample2 < 0) sourceSample2 += audioFileBuffer.getNumSamples();
                
                // Get audio samples (mono or averaged if stereo source)
                float audioSample1 = 0.0f;
                float audioSample2 = 0.0f;
                for (int channel = 0; channel < audioFileBuffer.getNumChannels(); ++channel)
                {
                    audioSample1 += audioFileBuffer.getSample (channel, sourceSample1);
                    audioSample2 += audioFileBuffer.getSample (channel, sourceSample2);
                }
                audioSample1 /= audioFileBuffer.getNumChannels();
                audioSample2 /= audioFileBuffer.getNumChannels();
                
                // Linear interpolation
                float audioSample = audioSample1 + fraction * (audioSample2 - audioSample1);
                
                // Write to output with panning (main + crossfade)
                if (totalNumOutputChannels >= 1)
                    buffer.addSample (0, i, audioSample * (leftGain + crossLeftGain));
                if (totalNumOutputChannels >= 2)
                    buffer.addSample (1, i, audioSample * (rightGain + crossRightGain));
            }
        }
    }
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    LOG_INFO("Creating plugin editor");
    return new PluginEditor (*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// Audio File Loading

void PluginProcessor::loadAudioFile (const juce::File& file)
{
    if (!file.existsAsFile())
    {
        LOG_WARNING("Attempted to load non-existent file: " + file.getFullPathName());
        return;
    }
    
    LOG_INFO("Loading audio file: " + file.getFullPathName());
    
    // Create audio format manager and register common formats
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    // Create reader for the file
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    
    if (reader != nullptr)
    {
        // Store file info
        loadedAudioFile = file;
        audioFileSampleRate = reader->sampleRate;
        
        // Read audio data into buffer
        audioFileBuffer.setSize (static_cast<int>(reader->numChannels),
                                 static_cast<int>(reader->lengthInSamples));
        
        reader->read (&audioFileBuffer,
                      0,
                      static_cast<int>(reader->lengthInSamples),
                      0,
                      true,
                      true);
        
        LOG_INFO("Audio file loaded successfully - Channels: " + juce::String(reader->numChannels) +
                 ", Sample Rate: " + juce::String(reader->sampleRate) + " Hz, " +
                 "Length: " + juce::String(reader->lengthInSamples) + " samples (" +
                 juce::String(reader->lengthInSamples / reader->sampleRate, 2) + " seconds)");
    }
    else
    {
        LOG_WARNING("Failed to create audio reader for file: " + file.getFullPathName());
        loadedAudioFile = juce::File();
        audioFileBuffer.setSize (0, 0);
        audioFileSampleRate = 0.0;
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
