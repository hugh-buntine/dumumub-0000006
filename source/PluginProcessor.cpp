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
    
    // Create default mass point at center (will be positioned correctly when canvas size is known)
    massPoints.push_back ({ juce::Point<float>(200.0f, 200.0f), 4.0f });
    
    // Create default spawn point at center with angle pointing right
    spawnPoints.push_back ({ juce::Point<float>(200.0f, 200.0f), 0.0f });
    
    LOG_INFO("Created default mass point and spawn point");
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

//==============================================================================
// Mass and spawn point management

void PluginProcessor::updateMassPoint (int index, juce::Point<float> position, float massMultiplier)
{
    if (index >= 0 && index < static_cast<int>(massPoints.size()))
    {
        massPoints[index].position = position;
        massPoints[index].massMultiplier = massMultiplier;
    }
}

void PluginProcessor::addMassPoint (juce::Point<float> position, float massMultiplier)
{
    MassPointData data;
    data.position = position;
    data.massMultiplier = massMultiplier;
    massPoints.push_back (data);
}

void PluginProcessor::removeMassPoint (int index)
{
    if (index >= 0 && index < static_cast<int>(massPoints.size()))
        massPoints.erase (massPoints.begin() + index);
}

void PluginProcessor::updateSpawnPoint (int index, juce::Point<float> position, float angle)
{
    if (index >= 0 && index < static_cast<int>(spawnPoints.size()))
    {
        spawnPoints[index].position = position;
        spawnPoints[index].momentumAngle = angle;
    }
}

void PluginProcessor::addSpawnPoint (juce::Point<float> position, float angle)
{
    SpawnPointData data;
    data.position = position;
    data.momentumAngle = angle;
    spawnPoints.push_back (data);
    LOG_INFO("Processor: Added spawn point - Total: " + juce::String(spawnPoints.size()) + 
        " at (" + juce::String(position.x) + ", " + juce::String(position.y) + 
        ") angle: " + juce::String(angle));
}

void PluginProcessor::removeSpawnPoint (int index)
{
    if (index >= 0 && index < static_cast<int>(spawnPoints.size()))
        spawnPoints.erase (spawnPoints.begin() + index);
}

void PluginProcessor::spawnParticle (juce::Point<float> position, juce::Point<float> velocity,
                                     float initialVelocity, float pitchShift)
{
    const juce::ScopedLock lock (particlesLock);
    
    // Check particle limit and remove oldest if at max
    if (particles.size() >= maxParticles)
    {
        // Remove oldest particle (index 0)
        particles.remove (0);
        LOG_INFO("Removed oldest particle to make room (max: " + juce::String(maxParticles) + ")");
    }
    
    auto* particle = new Particle (position, velocity, canvasBounds, 
                                   particleLifespan, initialVelocity, pitchShift);
    particles.add (particle);
}

//==============================================================================
// Particle simulation update

void PluginProcessor::updateParticleSimulation (double currentTime, int bufferSize)
{
    // Calculate time since last update
    if (lastUpdateTime == 0.0)
        lastUpdateTime = currentTime;
    
    float deltaTime = static_cast<float>(currentTime - lastUpdateTime);
    lastUpdateTime = currentTime;
    
    // Clamp delta time to avoid huge jumps
    deltaTime = juce::jmin (deltaTime, 0.1f);
    
    // Lock particles for update
    const juce::ScopedLock lock (particlesLock);
    
    // Update spawn point visual rotations (animation only, not used for particle spawning)
    for (auto& spawn : spawnPoints)
    {
        spawn.visualRotation += deltaTime * 0.5f; // Rotate at 0.5 rad/s
        if (spawn.visualRotation > juce::MathConstants<float>::twoPi)
            spawn.visualRotation -= juce::MathConstants<float>::twoPi;
    }
    
    // Update all particles
    for (int i = particles.size() - 1; i >= 0; --i)
    {
        auto* particle = particles[i];
        
        // Update canvas bounds
        particle->setCanvasBounds (canvasBounds);
        
        // Calculate gravity force from all mass points
        juce::Point<float> totalForce (0.0f, 0.0f);
        
        for (const auto& mass : massPoints)
        {
            juce::Point<float> particlePos = particle->getPosition();
            juce::Point<float> direction = mass.position - particlePos;
            float distance = direction.getDistanceFromOrigin();
            
            // Avoid division by zero and singularities
            if (distance > 5.0f)
            {
                // Apply mass multiplier based on size
                float effectiveGravity = gravityStrength * mass.massMultiplier;
                float forceMagnitude = effectiveGravity / (distance * distance);
                juce::Point<float> normalizedDirection = direction / distance;
                totalForce += normalizedDirection * forceMagnitude;
            }
        }
        
        // Apply gravity force
        particle->applyForce (totalForce);
        
        // Update particle physics
        particle->update (deltaTime);
        
        // Wrap around canvas boundaries
        particle->wrapAround (canvasBounds);
        
        // Remove dead particles
        if (particle->isDead())
            particles.remove (i);
    }
}

//==============================================================================

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

    // Merge in any pending MIDI messages from UI thread
    {
        const juce::ScopedLock lock (midiLock);
        if (!pendingMidiMessages.isEmpty())
        {
            LOG_INFO("Merging " + juce::String(pendingMidiMessages.getNumEvents()) + " pending MIDI messages into buffer");
            midiMessages.addEvents (pendingMidiMessages, 0, buffer.getNumSamples(), 0);
            pendingMidiMessages.clear();
        }
    }

    LOG_INFO("processBlock: About to process " + juce::String(midiMessages.getNumEvents()) + " MIDI messages, canvas=" + juce::String(canvas != nullptr ? "valid" : "null"));

    // Process MIDI messages to spawn particles
    // Note: canvas can still be used during transition for spawning logic
    if (canvas != nullptr)
    {
        for (const auto metadata : midiMessages)
        {
            LOG_INFO("Processing MIDI message in loop");
            auto message = metadata.getMessage();
            
            if (message.isNoteOn())
            {
                int midiNote = message.getNoteNumber();
                float midiVelocity = message.getVelocity() / 127.0f; // Normalize to 0.0-1.0
                
                LOG_INFO("MIDI Note On received: note=" + juce::String(midiNote) + " velocity=" + juce::String(midiVelocity));
                LOG_INFO("Spawn points available: " + juce::String(spawnPoints.size()));
                
                // Spawn particle directly from MIDI (particles now live on audio thread)
                // Get spawn point information from canvas (if available)
                if (canvas != nullptr && spawnPoints.size() > 0)
                {
                    LOG_INFO("Spawning particle from MIDI trigger...");
                    // Use round-robin spawn point selection
                    static int nextSpawnIndex = 0;
                    int spawnIndex = nextSpawnIndex % spawnPoints.size();
                    nextSpawnIndex = (nextSpawnIndex + 1) % spawnPoints.size();
                    
                    auto& spawn = spawnPoints[spawnIndex];
                    juce::Point<float> spawnPos = spawn.position;
                    
                    // Get momentum from spawn point's momentum angle (set by user via arrow drag)
                    float angle = spawn.momentumAngle;
                    float momentumMagnitude = 50.0f; // Default momentum
                    juce::Point<float> initialVelocity (
                        std::cos(angle) * momentumMagnitude,
                        std::sin(angle) * momentumMagnitude
                    );
                    initialVelocity *= 2.0f; // Scale up for visibility
                    
                    // Calculate pitch shift from MIDI note (C3 = 60 = no shift)
                    float semitoneOffset = midiNote - 60;
                    float pitchShift = std::pow (2.0f, semitoneOffset / 12.0f);
                    
                    // Spawn particle with MIDI parameters
                    spawnParticle (spawnPos, initialVelocity, midiVelocity, pitchShift);
                    LOG_INFO("Particle spawned! Total particles: " + juce::String(particles.size()));
                }
                else
                {
                    LOG_INFO("Cannot spawn particle - canvas=" + juce::String(canvas != nullptr ? "valid" : "null") + 
                        ", spawnPoints=" + juce::String(spawnPoints.size()));
                }
            }
        }
    }
    
    // Update particle simulation (physics, gravity, lifetime)
    double currentTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
    updateParticleSimulation (currentTime, buffer.getNumSamples());

    // Check if we have audio file loaded
    if (!hasAudioFileLoaded() || audioFileBuffer.getNumSamples() == 0)
        return;
    
    // Get parameter values
    float grainSizeMs = apvts.getRawParameterValue("grainSize")->load();
    float grainFreq = apvts.getRawParameterValue("grainFreq")->load();
    float attackMs = apvts.getRawParameterValue("attack")->load();
    float releaseMs = apvts.getRawParameterValue("release")->load();
    float masterGainDb = apvts.getRawParameterValue("masterGain")->load();
    float masterGainLinear = juce::Decibels::decibelsToGain(masterGainDb);
    
    // Lock particles for grain processing (already locked in updateParticleSimulation, but released)
    const juce::ScopedLock lock (particlesLock);
    
    if (particles.isEmpty())
        return;
    
    // Process each particle's grains
    for (auto* particle : particles)
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

void PluginProcessor::injectMidiMessage (const juce::MidiMessage& message)
{
    const juce::ScopedLock lock (midiLock);
    pendingMidiMessages.addEvent (message, 0);
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
    // Save plugin state to XML
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    
    // Add audio file path
    if (loadedAudioFile.existsAsFile())
    {
        xml->setAttribute ("audioFile", loadedAudioFile.getFullPathName());
    }
    
    // Save mass points
    auto* massPointsXml = xml->createNewChildElement ("MassPoints");
    for (const auto& mp : massPoints)
    {
        auto* mpXml = massPointsXml->createNewChildElement ("MassPoint");
        mpXml->setAttribute ("x", mp.position.x);
        mpXml->setAttribute ("y", mp.position.y);
        mpXml->setAttribute ("mass", mp.massMultiplier);
    }
    
    // Save spawn points
    auto* spawnPointsXml = xml->createNewChildElement ("SpawnPoints");
    for (const auto& sp : spawnPoints)
    {
        auto* spXml = spawnPointsXml->createNewChildElement ("SpawnPoint");
        spXml->setAttribute ("x", sp.position.x);
        spXml->setAttribute ("y", sp.position.y);
        spXml->setAttribute ("momentumAngle", sp.momentumAngle);  // Save user-set momentum direction (not visual rotation)
    }
    
    copyXmlToBinary (*xml, destData);
    LOG_INFO("Saved plugin state with " + juce::String(massPoints.size()) + " mass points, " +
             juce::String(spawnPoints.size()) + " spawn points");
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore plugin state from XML
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    
    if (xmlState != nullptr)
    {
        // Restore parameters
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
        }
        
        // Restore audio file
        if (xmlState->hasAttribute ("audioFile"))
        {
            juce::File audioFile (xmlState->getStringAttribute ("audioFile"));
            if (audioFile.existsAsFile())
            {
                loadAudioFile (audioFile);
                LOG_INFO("Restored audio file: " + audioFile.getFullPathName());
            }
            else
            {
                LOG_WARNING("Saved audio file not found: " + audioFile.getFullPathName());
            }
        }
        
        // Restore mass points
        auto* massPointsXml = xmlState->getChildByName ("MassPoints");
        if (massPointsXml != nullptr)
        {
            massPoints.clear();
            for (auto* mpXml : massPointsXml->getChildIterator())
            {
                if (mpXml->hasTagName ("MassPoint"))
                {
                    MassPointData mp;
                    mp.position.x = static_cast<float>(mpXml->getDoubleAttribute ("x"));
                    mp.position.y = static_cast<float>(mpXml->getDoubleAttribute ("y"));
                    mp.massMultiplier = static_cast<float>(mpXml->getDoubleAttribute ("mass"));
                    massPoints.push_back (mp);
                }
            }
            LOG_INFO("Restored " + juce::String(massPoints.size()) + " mass points");
        }
        
        // Restore spawn points
        auto* spawnPointsXml = xmlState->getChildByName ("SpawnPoints");
        if (spawnPointsXml != nullptr)
        {
            spawnPoints.clear();
            for (auto* spXml : spawnPointsXml->getChildIterator())
            {
                if (spXml->hasTagName ("SpawnPoint"))
                {
                    SpawnPointData sp;
                    sp.position.x = static_cast<float>(spXml->getDoubleAttribute ("x"));
                    sp.position.y = static_cast<float>(spXml->getDoubleAttribute ("y"));
                    sp.momentumAngle = static_cast<float>(spXml->getDoubleAttribute ("momentumAngle"));  // Load user-set momentum direction
                    spawnPoints.push_back (sp);
                }
            }
            LOG_INFO("Restored " + juce::String(spawnPoints.size()) + " spawn points");
        }
    }
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
