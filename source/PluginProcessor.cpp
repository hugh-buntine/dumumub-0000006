#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Logger.h"
#include "Canvas.h"
#include "Particle.h"
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
    Particle::initializeHannTable();
    
    auto& state = apvts.state;
    if (!state.getChildWithName("MassPoints").isValid())
        state.appendChild(juce::ValueTree("MassPoints"), nullptr);
    if (!state.getChildWithName("SpawnPoints").isValid())
        state.appendChild(juce::ValueTree("SpawnPoints"), nullptr);
    
    loadPointsFromTree();
    
    // Create defaults for fresh plugin (no saved state)
    if (massPoints.empty() && spawnPoints.empty())
    {
        massPoints.push_back({ juce::Point<float>(200.0f, 200.0f), 4.0f });
        spawnPoints.push_back({ juce::Point<float>(100.0f, 300.0f), 0.0f });
        savePointsToTree();
    }
}

PluginProcessor::~PluginProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Grain Size (10ms - 500ms, increased minimum)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "grainSize",
        "Grain Size",
        juce::NormalisableRange<float> (10.0f, 500.0f, 1.0f, 0.5f),
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
    
    // Attack Time (0.01s - 2.0s) - Now controls particle ADSR attack
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "attack",
        "Attack",
        juce::NormalisableRange<float> (0.01f, 2.0f, 0.001f, 0.25f),
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
    
    // Release Time (0.01s - 5.0s) - Now controls particle ADSR release
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "release",
        "Release",
        juce::NormalisableRange<float> (0.01f, 5.0f, 0.001f, 0.3f),
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
    
    // Decay Time (0.01s - 5.0s) - Controls particle ADSR decay
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "decay",
        "Decay",
        juce::NormalisableRange<float> (0.01f, 5.0f, 0.001f, 0.3f),
        0.3f,
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
    
    // Master Gain (-60dB - +6dB with special -infinity at leftmost position)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "masterGain",
        "Master Gain",
        juce::NormalisableRange<float> (-60.0f, 6.0f, 0.1f),
        -6.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { 
            if (value <= -60.0f) 
                return juce::String ("-∞");
            else 
                return juce::String (value, 1) + " dB"; 
        }
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
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void PluginProcessor::releaseResources()
{
}

//==============================================================================
void PluginProcessor::loadPointsFromTree()
{
    auto& state = apvts.state;
    
    auto massPointsTree = state.getChildWithName("MassPoints");
    if (massPointsTree.isValid())
    {
        massPoints.clear();
        for (int i = 0; i < massPointsTree.getNumChildren(); ++i)
        {
            auto child = massPointsTree.getChild(i);
            MassPointData mp;
            mp.position.x = child.getProperty("x", 200.0f);
            mp.position.y = child.getProperty("y", 200.0f);
            mp.massMultiplier = child.getProperty("mass", 4.0f);
            massPoints.push_back(mp);
        }
    }
    
    auto spawnPointsTree = state.getChildWithName("SpawnPoints");
    if (spawnPointsTree.isValid())
    {
        spawnPoints.clear();
        for (int i = 0; i < spawnPointsTree.getNumChildren(); ++i)
        {
            auto child = spawnPointsTree.getChild(i);
            SpawnPointData sp;
            sp.position.x = child.getProperty("x", 200.0f);
            sp.position.y = child.getProperty("y", 200.0f);
            sp.momentumAngle = child.getProperty("angle", 0.0f);
            spawnPoints.push_back(sp);
        }
    }
}

void PluginProcessor::savePointsToTree()
{
    auto& state = apvts.state;
    
    auto massPointsTree = state.getOrCreateChildWithName("MassPoints", nullptr);
    massPointsTree.removeAllChildren(nullptr);
    for (const auto& mp : massPoints)
    {
        juce::ValueTree child("MassPoint");
        child.setProperty("x", mp.position.x, nullptr);
        child.setProperty("y", mp.position.y, nullptr);
        child.setProperty("mass", mp.massMultiplier, nullptr);
        massPointsTree.appendChild(child, nullptr);
    }
    
    auto spawnPointsTree = state.getOrCreateChildWithName("SpawnPoints", nullptr);
    spawnPointsTree.removeAllChildren(nullptr);
    for (const auto& sp : spawnPoints)
    {
        juce::ValueTree child("SpawnPoint");
        child.setProperty("x", sp.position.x, nullptr);
        child.setProperty("y", sp.position.y, nullptr);
        child.setProperty("angle", sp.momentumAngle, nullptr);
        spawnPointsTree.appendChild(child, nullptr);
    }
}

//==============================================================================
void PluginProcessor::updateMassPoint (int index, juce::Point<float> position, float massMultiplier)
{
    if (index >= 0 && index < static_cast<int>(massPoints.size()))
    {
        massPoints[static_cast<size_t>(index)].position = position;
        massPoints[static_cast<size_t>(index)].massMultiplier = massMultiplier;
        savePointsToTree();
        updateHostDisplay();
    }
}

void PluginProcessor::addMassPoint (juce::Point<float> position, float massMultiplier)
{
    MassPointData data;
    data.position = position;
    data.massMultiplier = massMultiplier;
    massPoints.push_back (data);
    savePointsToTree();
    updateHostDisplay();
}

void PluginProcessor::removeMassPoint (int index)
{
    if (index >= 0 && index < static_cast<int>(massPoints.size()))
    {
        massPoints.erase (massPoints.begin() + index);
        savePointsToTree();
        updateHostDisplay();
    }
}

void PluginProcessor::updateSpawnPoint (int index, juce::Point<float> position, float angle)
{
    if (index >= 0 && index < static_cast<int>(spawnPoints.size()))
    {
        spawnPoints[static_cast<size_t>(index)].position = position;
        spawnPoints[static_cast<size_t>(index)].momentumAngle = angle;
        savePointsToTree();
        updateHostDisplay();
    }
}

void PluginProcessor::addSpawnPoint (juce::Point<float> position, float angle)
{
    SpawnPointData data;
    data.position = position;
    data.momentumAngle = angle;
    spawnPoints.push_back (data);
    savePointsToTree();
    updateHostDisplay();
}

void PluginProcessor::removeSpawnPoint (int index)
{
    if (index >= 0 && index < static_cast<int>(spawnPoints.size()))
    {
        spawnPoints.erase (spawnPoints.begin() + index);
        savePointsToTree();
        updateHostDisplay();
    }
}

void PluginProcessor::spawnParticle (juce::Point<float> position, juce::Point<float> velocity,
                                     float initialVelocity, float pitchShift, int midiNoteNumber,
                                     float attackTime, float sustainLevel, float sustainLevelLinear, float releaseTime)
{
    const juce::ScopedLock lock (particlesLock);
    
    // Remove oldest particle if at limit
    if (particles.size() >= maxParticles)
    {
        auto* oldestParticle = particles[0];
        int oldNote = oldestParticle->getMidiNoteNumber();
        
        if (activeNoteToParticles.count(oldNote) > 0)
        {
            auto& particleIndices = activeNoteToParticles[oldNote];
            particleIndices.erase(std::remove(particleIndices.begin(), particleIndices.end(), 0), 
                                 particleIndices.end());
            if (particleIndices.empty())
                activeNoteToParticles.erase(oldNote);
        }
        
        particles.remove (0);
        
        // Shift all indices down
        for (auto& pair : activeNoteToParticles)
        {
            for (auto& idx : pair.second)
                if (idx > 0) idx--;
        }
    }
    
    auto* particle = new Particle (position, velocity, canvasBounds, midiNoteNumber,
                                   attackTime, sustainLevel, sustainLevelLinear, releaseTime, initialVelocity, pitchShift);
    particle->setBounceMode (bounceMode);
    int newIndex = particles.size();
    particles.add (particle);
    
    activeNoteToParticles[midiNoteNumber].push_back(newIndex);
}

//==============================================================================
void PluginProcessor::handleNoteOn (int noteNumber, float velocity, float pitchShift)
{
    // Create defaults if needed
    if (spawnPoints.size() == 0)
        spawnPoints.push_back ({ juce::Point<float>(200.0f, 200.0f), 0.0f });
    
    if (massPoints.size() == 0)
        massPoints.push_back ({ juce::Point<float>(200.0f, 200.0f), 2.0f });
    
    float attackTime = apvts.getRawParameterValue("attack")->load();
    float sustainLevelLinear = apvts.getRawParameterValue("sustain")->load();
    float releaseTime = apvts.getRawParameterValue("release")->load();
    
    // Convert linear sustain (0-1) to logarithmic amplitude (-60dB to 0dB)
    float sustainLevel;
    if (sustainLevelLinear < 0.001f)
    {
        sustainLevel = 0.0f;
    }
    else
    {
        float sustainDb = (sustainLevelLinear - 1.0f) * 60.0f;
        sustainLevel = juce::Decibels::decibelsToGain(sustainDb);
    }
    
    // Round-robin spawn point selection
    static size_t nextSpawnIndex = 0;
    size_t spawnIndex = nextSpawnIndex % spawnPoints.size();
    nextSpawnIndex = (nextSpawnIndex + 1) % spawnPoints.size();
    
    auto& spawn = spawnPoints[spawnIndex];
    juce::Point<float> spawnPos = spawn.position;
    
    float momentumMagnitude = 50.0f;
    juce::Point<float> initialVelocity (
        std::cos(spawn.momentumAngle) * momentumMagnitude,
        std::sin(spawn.momentumAngle) * momentumMagnitude
    );
    initialVelocity *= 2.0f;
    
    spawnParticle (spawnPos, initialVelocity, velocity, pitchShift, noteNumber, attackTime, sustainLevel, sustainLevelLinear, releaseTime);
}

void PluginProcessor::handleNoteOff (int noteNumber)
{
    const juce::ScopedLock lock (particlesLock);
    
    if (activeNoteToParticles.count(noteNumber) > 0)
    {
        auto& particleIndices = activeNoteToParticles[noteNumber];
        
        for (int idx : particleIndices)
        {
            if (idx >= 0 && idx < particles.size())
            {
                particles[idx]->triggerRelease();
            }
        }
    }
}

//==============================================================================
void PluginProcessor::updateParticleSimulation (double currentTime, int /*bufferSize*/)
{
    if (lastUpdateTime == 0.0)
        lastUpdateTime = currentTime;
    
    float deltaTime = static_cast<float>(currentTime - lastUpdateTime);
    lastUpdateTime = currentTime;
    deltaTime = juce::jmin (deltaTime, 0.1f);
    
    const juce::ScopedLock lock (particlesLock);
    
    // Rotate spawn points for visual animation
    for (auto& spawn : spawnPoints)
    {
        spawn.visualRotation += deltaTime * 0.5f;
        if (spawn.visualRotation > juce::MathConstants<float>::twoPi)
            spawn.visualRotation -= juce::MathConstants<float>::twoPi;
    }
    
    for (int i = particles.size() - 1; i >= 0; --i)
    {
        auto* particle = particles[i];
        particle->setCanvasBounds (canvasBounds);
        
        // Calculate gravity from mass points
        juce::Point<float> totalForce (0.0f, 0.0f);
        
        for (const auto& mass : massPoints)
        {
            juce::Point<float> particlePos = particle->getPosition();
            juce::Point<float> direction = mass.position - particlePos;
            float distance = direction.getDistanceFromOrigin();
            
            if (distance > 5.0f)
            {
                float effectiveGravity = gravityStrength * mass.massMultiplier;
                float forceMagnitude = effectiveGravity / (distance * distance);
                juce::Point<float> normalizedDirection = direction / distance;
                totalForce += normalizedDirection * forceMagnitude;
            }
        }
        
        particle->applyForce (totalForce);
        particle->update (deltaTime);
        
        if (bounceMode)
            particle->bounceOff (canvasBounds);
        else
            particle->wrapAround (canvasBounds);
        
        // Remove finished particles
        if (particle->isFinished())
        {
            int noteNumber = particle->getMidiNoteNumber();
            
            if (activeNoteToParticles.count(noteNumber) > 0)
            {
                auto& particleIndices = activeNoteToParticles[noteNumber];
                particleIndices.erase(std::remove(particleIndices.begin(), particleIndices.end(), i), 
                                     particleIndices.end());
                if (particleIndices.empty())
                    activeNoteToParticles.erase(noteNumber);
            }
            
            particles.remove (i);
            
            // Shift indices
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
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

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
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = 0; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    // Merge pending MIDI from UI thread
    {
        const juce::ScopedLock lock (midiLock);
        if (!pendingMidiMessages.isEmpty())
        {
            midiMessages.addEvents (pendingMidiMessages, 0, buffer.getNumSamples(), 0);
            pendingMidiMessages.clear();
        }
    }

    // Process MIDI
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        
        if (message.isNoteOn())
        {
            int midiNote = message.getNoteNumber();
            float midiVelocity = message.getVelocity() / 127.0f;
            float semitoneOffset = midiNote - 60;
            float pitchShift = std::pow (2.0f, semitoneOffset / 12.0f);
            handleNoteOn (midiNote, midiVelocity, pitchShift);
        }
        else if (message.isNoteOff())
        {
            handleNoteOff (message.getNoteNumber());
        }
    }
    
    double currentTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
    updateParticleSimulation (currentTime, buffer.getNumSamples());

    if (!hasAudioFileLoaded() || audioFileBuffer.getNumSamples() == 0)
        return;
    
    float grainSizeMs = apvts.getRawParameterValue("grainSize")->load();
    float grainFreq = apvts.getRawParameterValue("grainFreq")->load();
    float masterGainDb = apvts.getRawParameterValue("masterGain")->load();
    
    float masterGainLinear;
    if (masterGainDb <= -60.0f)
        masterGainLinear = 0.0f;
    else
        masterGainLinear = juce::Decibels::decibelsToGain(masterGainDb);
    
    const juce::ScopedLock lock (particlesLock);
    
    if (particles.isEmpty())
        return;
    
    // Automatic gain compensation for overlapping grains
    int totalActiveGrains = 0;
    for (auto* particle : particles)
        totalActiveGrains += particle->getActiveGrains().size();
    
    float targetGainCompensation = 1.0f;
    if (totalActiveGrains > 1)
    {
        targetGainCompensation = 1.0f / std::sqrt(static_cast<float>(totalActiveGrains));
        targetGainCompensation = juce::jmax(0.1f, targetGainCompensation);
    }
    
    // Smooth gain changes to prevent clicks
    static float smoothedGainCompensation = 1.0f;
    float gainDifference = std::abs(targetGainCompensation - smoothedGainCompensation);
    float relativeDifference = gainDifference / juce::jmax(0.01f, smoothedGainCompensation);
    float timeConstant = 0.010f + (relativeDifference * 0.040f);
    timeConstant = juce::jmin(0.050f, timeConstant);
    float smoothingCoefficient = 1.0f - static_cast<float>(std::exp(-2.2 / (static_cast<double>(timeConstant) * getSampleRate())));
    smoothedGainCompensation += smoothingCoefficient * (targetGainCompensation - smoothedGainCompensation);
    float gainCompensation = smoothedGainCompensation;
    
    for (auto* particle : particles)
    {
        particle->updateSampleRate (getSampleRate());
        particle->setGrainParameters (grainSizeMs, 0.0f, 0.0f);
        
        if (particle->shouldTriggerNewGrain (getSampleRate(), grainFreq))
            particle->triggerNewGrain (audioFileBuffer.getNumSamples());
        
        auto& grains = particle->getActiveGrains();
        
        for (auto& grain : grains)
            grain.samplesRenderedThisBuffer = 0;
        
        if (grains.empty())
        {
            particle->updateGrains (buffer.getNumSamples());
            continue;
        }
        
        // Pre-calculate ADSR for entire buffer
        std::vector<float> adsrAmplitudes(static_cast<size_t>(buffer.getNumSamples()));
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            particle->updateADSRSample (getSampleRate());
            adsrAmplitudes[static_cast<size_t>(i)] = particle->getADSRAmplitudeSmoothed();
        }
        
        for (auto& grain : grains)
        {
            int grainStartSample = grain.startSample;
            int grainPosition = grain.playbackPosition;
            int totalGrainSamples = particle->getTotalGrainSamples();
            
            int samplesToRender = juce::jmin (buffer.getNumSamples(), 
                                              totalGrainSamples - grainPosition);
            
            if (samplesToRender <= 0)
                continue;
            
            auto edgeFade = particle->getEdgeFade();
            
            float constantAmplitude = masterGainLinear;
            constantAmplitude *= edgeFade.amplitude;
            constantAmplitude *= particle->getInitialVelocityMultiplier();
            constantAmplitude *= gainCompensation;
            
            float pitchShift = particle->getPitchShift();
            
            float panAngle = (edgeFade.pan + 1.0f) * juce::MathConstants<float>::pi / 4.0f;
            float leftPanGain = std::cos (panAngle);
            float rightPanGain = std::sin (panAngle);
            
            float channelMult = 1.0f / static_cast<float>(audioFileBuffer.getNumChannels());
            int bufferLength = audioFileBuffer.getNumSamples();
            
            const float** sourceChannelPointers = new const float*[static_cast<size_t>(audioFileBuffer.getNumChannels())];
            for (int ch = 0; ch < audioFileBuffer.getNumChannels(); ++ch)
                sourceChannelPointers[ch] = audioFileBuffer.getReadPointer (ch);
            
            float* leftChannel = totalNumOutputChannels >= 1 ? buffer.getWritePointer (0) : nullptr;
            float* rightChannel = totalNumOutputChannels >= 2 ? buffer.getWritePointer (1) : nullptr;
            
            for (int i = 0; i < samplesToRender; ++i)
            {
                float sourcePosition = grainStartSample + ((grainPosition + i) * pitchShift);
                
                // Wrap around buffer boundaries
                while (sourcePosition >= bufferLength)
                    sourcePosition -= bufferLength;
                while (sourcePosition < 0)
                    sourcePosition += bufferLength;
                
                int sourceSample = static_cast<int>(sourcePosition);
                float fraction = sourcePosition - sourceSample;
                fraction = juce::jlimit(0.0f, 1.0f, fraction);
                
                // Cubic interpolation indices with wrapping
                auto wrapIndex = [bufferLength](int index) -> int {
                    while (index < 0) index += bufferLength;
                    while (index >= bufferLength) index -= bufferLength;
                    return index;
                };
                
                int s0 = wrapIndex(sourceSample - 1);
                int s1 = wrapIndex(sourceSample);
                int s2 = wrapIndex(sourceSample + 1);
                int s3 = wrapIndex(sourceSample + 2);
                
                float y0 = 0.0f, y1 = 0.0f, y2 = 0.0f, y3 = 0.0f;
                for (int channel = 0; channel < audioFileBuffer.getNumChannels(); ++channel)
                {
                    y0 += sourceChannelPointers[channel][s0];
                    y1 += sourceChannelPointers[channel][s1];
                    y2 += sourceChannelPointers[channel][s2];
                    y3 += sourceChannelPointers[channel][s3];
                }
                y0 *= channelMult;
                y1 *= channelMult;
                y2 *= channelMult;
                y3 *= channelMult;
                
                // Cubic Hermite interpolation
                float c0 = y1;
                float c1 = 0.5f * (y2 - y0);
                float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
                float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
                float audioSample = ((c3 * fraction + c2) * fraction + c1) * fraction + c0;
                
                // Clamp to prevent cubic overshoot
                float minSample = juce::jmin(y0, y1, y2, y3);
                float maxSample = juce::jmax(y0, y1, y2, y3);
                audioSample = juce::jlimit(minSample, maxSample, audioSample);
                
                // Flush denormals
                if (std::abs(audioSample) < 1e-6f)
                    audioSample = 0.0f;
                
                // Calculate grain amplitude (Hann window)
                Grain currentGrain = grain;
                currentGrain.playbackPosition = grainPosition + i;
                float grainAmplitude = particle->getGrainAmplitude (currentGrain);
                
                float adsrAmplitude = adsrAmplitudes[static_cast<size_t>(i)];
                float totalAmplitude = grainAmplitude * constantAmplitude * adsrAmplitude;
                
                float leftGain = leftPanGain * totalAmplitude;
                float rightGain = rightPanGain * totalAmplitude;
                
                float leftSample = audioSample * leftGain;
                float rightSample = audioSample * rightGain;
                
                // Soft clip to prevent harsh digital clipping
                if (std::abs(leftSample) > 0.9f)
                    leftSample = std::tanh(leftSample * 0.9f) / 0.9f;
                if (std::abs(rightSample) > 0.9f)
                    rightSample = std::tanh(rightSample * 0.9f) / 0.9f;
                
                if (leftChannel)
                    leftChannel[i] += leftSample;
                if (rightChannel)
                    rightChannel[i] += rightSample;
            }
            
            grain.samplesRenderedThisBuffer = samplesToRender;
            delete[] sourceChannelPointers;
        }
        
        particle->updateGrains (buffer.getNumSamples());
    }
    
    // Store output for continuity checking
    if (totalNumOutputChannels >= 1)
    {
        float* leftChannel = buffer.getWritePointer(0);
        lastBufferOutputLeft = leftChannel[buffer.getNumSamples() - 1];
        
        if (totalNumOutputChannels >= 2)
        {
            float* rightChannel = buffer.getWritePointer(1);
            lastBufferOutputRight = rightChannel[buffer.getNumSamples() - 1];
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
    // The ValueTree already contains the mass/spawn points (synced via savePointsToTree())
    // Just save the APVTS state which includes everything
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    
    // Add audio file path
    if (loadedAudioFile.existsAsFile())
    {
        xml->setAttribute ("audioFile", loadedAudioFile.getFullPathName());
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
        // Restore parameters (this automatically restores the ValueTree with mass/spawn points)
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
        
        // Load mass/spawn points from the restored ValueTree into the arrays
        loadPointsFromTree();
        
        // Mark that we've been through setStateInformation
        stateHasBeenRestored = true;
        
        // Don't create defaults here - use whatever was saved (even if 0 points)
        // Defaults are only created in constructor for brand new plugins
    }
}

//==============================================================================
void PluginProcessor::setBounceMode (bool enabled)
{
    bounceMode = enabled;
    
    const juce::ScopedLock lock (particlesLock);
    for (auto* particle : particles)
    {
        if (particle != nullptr)
            particle->setBounceMode (enabled);
    }
}

//==============================================================================

void PluginProcessor::loadAudioFile (const juce::File& file)
{
    if (!file.existsAsFile())
    {
        LOG_WARNING("Attempted to load non-existent file: " + file.getFullPathName());
        return;
    }
    
    LOG_INFO("Loading audio file: " + file.getFullPathName());
    
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    
    if (reader != nullptr)
    {
        loadedAudioFile = file;
        audioFileSampleRate = reader->sampleRate;
        
        audioFileBuffer.setSize (static_cast<int>(reader->numChannels),
                                 static_cast<int>(reader->lengthInSamples));
        
        reader->read (&audioFileBuffer,
                      0,
                      static_cast<int>(reader->lengthInSamples),
                      0,
                      true,
                      true);
        
        LOG_INFO("Audio file loaded - " + juce::String(reader->numChannels) + " ch, " +
                 juce::String(reader->sampleRate) + " Hz, " +
                 juce::String(reader->lengthInSamples / reader->sampleRate, 2) + "s");
    }
    else
    {
        LOG_WARNING("Failed to create audio reader for: " + file.getFullPathName());
        loadedAudioFile = juce::File();
        audioFileBuffer.setSize (0, 0);
        audioFileSampleRate = 0.0;
    }
}

//==============================================================================

void PluginProcessor::parameterChanged (const juce::String& /*parameterID*/, float /*newValue*/)
{
}

//==============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
