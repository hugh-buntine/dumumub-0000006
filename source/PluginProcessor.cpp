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
    
    // Grain Size (10ms - 250ms, reduced maximum for CPU efficiency)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "grainSize",
        "Grain Size",
        juce::NormalisableRange<float> (10.0f, 250.0f, 1.0f, 0.5f),
        50.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String (value, 1) + " ms"; }
    ));
    
    // Grain Frequency (5 - 50 Hz, narrower range)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "grainFreq",
        "Grain Frequency",
        juce::NormalisableRange<float> (5.0f, 50.0f, 0.1f, 0.4f),
        20.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String (value, 1) + " Hz"; }
    ));
    
    // Attack Time (0.001s - 2.0s) - Now controls particle ADSR attack
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "attack",
        "Attack",
        juce::NormalisableRange<float> (0.001f, 2.0f, 0.001f, 0.25f),
        0.01f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { 
            if (value >= 0.1f)
                return juce::String (value, 2) + " s";
            else
                return juce::String (value * 1000.0f, 0) + " ms";
        }
    ));
    
    // Release Time (0.001s - 5.0s) - Now controls particle ADSR release
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "release",
        "Release",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.3f),
        0.5f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { 
            if (value >= 0.1f)
                return juce::String (value, 2) + " s";
            else
                return juce::String (value * 1000.0f, 0) + " ms";
        }
    ));
    
    // Sustain Level (0.0 - 1.0) - Controls envelope sustain amplitude
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "sustain",
        "Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.7f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String (static_cast<int>(value * 100.0f)) + " %"; }
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
                                     float initialVelocity, float pitchShift, int midiNoteNumber,
                                     float attackTime, float sustainLevel, float releaseTime)
{
    const juce::ScopedLock lock (particlesLock);
    
    // Check particle limit and remove oldest if at max
    if (particles.size() >= maxParticles)
    {
        // Remove oldest particle (index 0) and clean up note mapping
        auto* oldestParticle = particles[0];
        int oldNote = oldestParticle->getMidiNoteNumber();
        
        // Remove from note mapping
        if (activeNoteToParticles.count(oldNote) > 0)
        {
            auto& particleIndices = activeNoteToParticles[oldNote];
            particleIndices.erase(std::remove(particleIndices.begin(), particleIndices.end(), 0), 
                                 particleIndices.end());
            if (particleIndices.empty())
                activeNoteToParticles.erase(oldNote);
        }
        
        particles.remove (0);
        LOG_INFO("Removed oldest particle to make room (max: " + juce::String(maxParticles) + ")");
        
        // Decrement all particle indices in the mapping since we removed index 0
        for (auto& pair : activeNoteToParticles)
        {
            for (auto& idx : pair.second)
                if (idx > 0) idx--;
        }
    }
    
    // Create new particle with ADSR parameters
    auto* particle = new Particle (position, velocity, canvasBounds, midiNoteNumber,
                                   attackTime, sustainLevel, releaseTime, initialVelocity, pitchShift);
    int newIndex = particles.size();
    particles.add (particle);
    
    // Add to note mapping for release handling
    activeNoteToParticles[midiNoteNumber].push_back(newIndex);
}

//==============================================================================
// MIDI Note Handlers

void PluginProcessor::handleNoteOn (int noteNumber, float velocity, float pitchShift)
{
    LOG_INFO("handleNoteOn: note=" + juce::String(noteNumber) + " velocity=" + juce::String(velocity));
    
    if (spawnPoints.size() == 0)
    {
        LOG_WARNING("No spawn points available for note-on");
        return;
    }
    
    // Get ADSR parameters
    float attackTime = apvts.getRawParameterValue("attack")->load();
    float sustainLevel = apvts.getRawParameterValue("sustain")->load();
    float releaseTime = apvts.getRawParameterValue("release")->load();
    
    // Use round-robin spawn point selection
    static int nextSpawnIndex = 0;
    int spawnIndex = nextSpawnIndex % spawnPoints.size();
    nextSpawnIndex = (nextSpawnIndex + 1) % spawnPoints.size();
    
    auto& spawn = spawnPoints[spawnIndex];
    juce::Point<float> spawnPos = spawn.position;
    
    // Use exact momentum angle from spawn point (no randomness)
    float finalAngle = spawn.momentumAngle;
    
    float momentumMagnitude = 50.0f;
    juce::Point<float> initialVelocity (
        std::cos(finalAngle) * momentumMagnitude,
        std::sin(finalAngle) * momentumMagnitude
    );
    initialVelocity *= 2.0f; // Scale up for visibility
    
    // Spawn particle with MIDI parameters and ADSR
    spawnParticle (spawnPos, initialVelocity, velocity, pitchShift, noteNumber, attackTime, sustainLevel, releaseTime);
    LOG_INFO("Particle spawned from note-on! Total particles: " + juce::String(particles.size()));
}

void PluginProcessor::handleNoteOff (int noteNumber)
{
    LOG_INFO("handleNoteOff: note=" + juce::String(noteNumber));
    
    const juce::ScopedLock lock (particlesLock);
    
    // Find all particles associated with this MIDI note and trigger release
    if (activeNoteToParticles.count(noteNumber) > 0)
    {
        auto& particleIndices = activeNoteToParticles[noteNumber];
        LOG_INFO("Triggering release for " + juce::String(particleIndices.size()) + " particles");
        
        for (int idx : particleIndices)
        {
            if (idx >= 0 && idx < particles.size())
            {
                particles[idx]->triggerRelease();
            }
        }
        
        // Don't erase from map yet - particles will be removed when release completes
    }
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
        
        // Remove finished particles (ADSR release complete)
        if (particle->isFinished())
        {
            int noteNumber = particle->getMidiNoteNumber();
            
            // Clean up note mapping
            if (activeNoteToParticles.count(noteNumber) > 0)
            {
                auto& particleIndices = activeNoteToParticles[noteNumber];
                particleIndices.erase(std::remove(particleIndices.begin(), particleIndices.end(), i), 
                                     particleIndices.end());
                if (particleIndices.empty())
                    activeNoteToParticles.erase(noteNumber);
            }
            
            particles.remove (i);
            
            // Decrement all particle indices greater than i in the mapping
            for (auto& pair : activeNoteToParticles)
            {
                for (auto& idx : pair.second)
                    if (idx > i) idx--;
            }
        }
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

    // Process MIDI messages to spawn particles or trigger release
    for (const auto metadata : midiMessages)
    {
        LOG_INFO("Processing MIDI message in loop");
        auto message = metadata.getMessage();
        
        if (message.isNoteOn())
        {
            int midiNote = message.getNoteNumber();
            float midiVelocity = message.getVelocity() / 127.0f; // Normalize to 0.0-1.0
            
            LOG_INFO("MIDI Note On received: note=" + juce::String(midiNote) + " velocity=" + juce::String(midiVelocity));
            
            // Calculate pitch shift from MIDI note (C3 = 60 = no shift)
            float semitoneOffset = midiNote - 60;
            float pitchShift = std::pow (2.0f, semitoneOffset / 12.0f);
            
            // Spawn particle with ADSR control
            handleNoteOn (midiNote, midiVelocity, pitchShift);
        }
        else if (message.isNoteOff())
        {
            int midiNote = message.getNoteNumber();
            
            LOG_INFO("MIDI Note Off received: note=" + juce::String(midiNote));
            
            // Trigger release phase for all particles associated with this note
            handleNoteOff (midiNote);
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
    float masterGainDb = apvts.getRawParameterValue("masterGain")->load();
    float masterGainLinear = juce::Decibels::decibelsToGain(masterGainDb);
    
    // Note: attack and release are now ADSR parameters, not grain envelope parameters
    // Grain crossfade is hardcoded to 50% in Particle::getGrainAmplitude()
    
    // Lock particles for grain processing (already locked in updateParticleSimulation, but released)
    const juce::ScopedLock lock (particlesLock);
    
    if (particles.isEmpty())
        return;
    
    // Process each particle's grains
    for (auto* particle : particles)
    {
        // Update sample rate if needed (cached internally)
        particle->updateSampleRate (getSampleRate());
        
        // Update grain parameters (attack/release ignored - kept for API compatibility)
        particle->setGrainParameters (grainSizeMs, 0.0f, 0.0f);
        
        // Check if we should trigger a new grain based on frequency
        if (particle->shouldTriggerNewGrain (getSampleRate(), grainFreq))
        {
            particle->triggerNewGrain (audioFileBuffer.getNumSamples());
        }
        
        // Update all grains (advance playback, remove finished)
        particle->updateGrains (buffer.getNumSamples());
        
        // Get all active grains for this particle
        auto& grains = particle->getActiveGrains();
        
        // Skip particles with no active grains (silent - no audio output)
        // IMPORTANT: Check AFTER triggering/updating grains so new particles can start!
        if (grains.empty())
            continue;
        
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
            
            // Get edge fade info (simple amplitude fade at boundaries)
            auto edgeFade = particle->getEdgeFade();
            
            // Get grain amplitude (includes hardcoded 50% crossfade AND particle ADSR)
            float amplitude = particle->getGrainAmplitude (grain); // 0.0 to 1.0
            
            // Apply edge fade (fades to 0 near boundaries)
            amplitude *= edgeFade.amplitude;
            
            // Apply MIDI velocity
            amplitude *= particle->getInitialVelocityMultiplier();
            
            // Apply master gain
            amplitude *= masterGainLinear;
            
            // CPU Optimization: Skip grains below audible threshold (saves ~30% CPU)
            if (amplitude < 0.01f)
                continue;
            
            // Get pitch shift for this particle
            float pitchShift = particle->getPitchShift();
            
            // Calculate stereo pan gains ONCE per grain (cache trig functions)
            float panAngle = (edgeFade.pan + 1.0f) * juce::MathConstants<float>::pi / 4.0f;
            float leftPanGain = std::cos (panAngle);
            float rightPanGain = std::sin (panAngle);
            
            // Pre-calculate channel averaging multiplier (faster than division)
            float channelMult = 1.0f / static_cast<float>(audioFileBuffer.getNumChannels());
            
            // Cache buffer length for fast wrapping
            int bufferLength = audioFileBuffer.getNumSamples();
            
            // CPU Optimization: Get direct read pointers for faster access (no virtual function calls)
            const float** sourceChannelPointers = new const float*[audioFileBuffer.getNumChannels()];
            for (int ch = 0; ch < audioFileBuffer.getNumChannels(); ++ch)
                sourceChannelPointers[ch] = audioFileBuffer.getReadPointer (ch);
            
            // Get output write pointers
            float* leftChannel = totalNumOutputChannels >= 1 ? buffer.getWritePointer (0) : nullptr;
            float* rightChannel = totalNumOutputChannels >= 2 ? buffer.getWritePointer (1) : nullptr;
            
            // Render grain samples with pitch shift
            for (int i = 0; i < samplesToRender; ++i)
            {
                // Calculate source position with pitch shift (float for interpolation)
                float sourcePosition = grainStartSample + ((grainPosition + i) * pitchShift);
                
                // Linear interpolation between samples for smooth pitch shift
                int sourceSample1 = static_cast<int>(sourcePosition);
                int sourceSample2 = sourceSample1 + 1;
                float fraction = sourcePosition - sourceSample1;
                
                // Fast wrapping with branches instead of modulo (branch prediction makes this faster)
                if (sourceSample1 >= bufferLength)
                    sourceSample1 -= bufferLength;
                else if (sourceSample1 < 0)
                    sourceSample1 += bufferLength;
                
                if (sourceSample2 >= bufferLength)
                    sourceSample2 -= bufferLength;
                else if (sourceSample2 < 0)
                    sourceSample2 += bufferLength;
                
                // Get audio samples using direct pointer access (much faster than getSample())
                float audioSample1 = 0.0f;
                float audioSample2 = 0.0f;
                for (int channel = 0; channel < audioFileBuffer.getNumChannels(); ++channel)
                {
                    audioSample1 += sourceChannelPointers[channel][sourceSample1];
                    audioSample2 += sourceChannelPointers[channel][sourceSample2];
                }
                audioSample1 *= channelMult;  // Multiply instead of divide (faster!)
                audioSample2 *= channelMult;
                
                // Linear interpolation
                float audioSample = audioSample1 + fraction * (audioSample2 - audioSample1);
                
                // Calculate final stereo gains (apply amplitude modulation to cached pan gains)
                float leftGain = leftPanGain * amplitude;
                float rightGain = rightPanGain * amplitude;
                
                // Write to output with panning using direct pointer access
                if (leftChannel)
                    leftChannel[i] += audioSample * leftGain;
                if (rightChannel)
                    rightChannel[i] += audioSample * rightGain;
            }
            
            // Clean up temporary pointer array
            delete[] sourceChannelPointers;
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
