#include "Canvas.h"
#include "Logger.h"
#include "PluginProcessor.h"

//==============================================================================
Canvas::Canvas (PluginProcessor& processor)
    : audioProcessor (processor)
{
    LOG_INFO("Canvas created");
    setSize (400, 400);
    
    audioProcessor.setCanvasBounds (getLocalBounds().toFloat());
    audioProcessor.setGravityStrength (gravityStrength);
    
    syncGuiFromProcessor();
    startTimer (1000 / 60);
}

Canvas::~Canvas()
{
    stopTimer();
    LOG_INFO("Canvas destroyed");
}

//==============================================================================
juce::OwnedArray<Particle>* Canvas::getParticles()
{
    return audioProcessor.getParticles();
}

juce::CriticalSection& Canvas::getParticlesLock()
{
    return audioProcessor.getParticlesLock();
}

//==============================================================================
void Canvas::setBounceMode (bool enabled)
{
    bounceMode = enabled;
    audioProcessor.setBounceMode (enabled);
    LOG_INFO("Bounce mode " + juce::String(enabled ? "enabled" : "disabled"));
}

//==============================================================================
void Canvas::paint (juce::Graphics& g)
{
    // Show "drop" hint when dragging audio file
    if (isDraggingFile && customTypeface != nullptr)
    {
        g.setColour (juce::Colour (0xFF, 0xFF, 0xF2).withAlpha (0.4f));
        auto font = juce::Font (juce::FontOptions (customTypeface).withHeight (80.0f));
        g.setFont (font);
        g.drawText ("drop", 
                   juce::Rectangle<float>(0.0f, 0.0f, (float)getWidth(), (float)getHeight()), 
                   juce::Justification::centred, true);
    }
    
    // Draw layers back to front
    if (showGravityWaves)
        drawGravityWaves (g);
    drawWaveform (g);
    drawParticles (g);
    drawMomentumArrows (g);
}

void Canvas::resized()
{
    audioProcessor.setCanvasBounds (getLocalBounds().toFloat());
}

void Canvas::newMassPoint()
{
    if (massPoints.size() >= maxMassPoints)
    {
        LOG_WARNING("Maximum mass points reached (" + juce::String(maxMassPoints) + ")");
        return;
    }
    
    juce::Random random;
    float x = getWidth() > 0 ? random.nextFloat() * getWidth() : 100.0f;
    float y = getHeight() > 0 ? random.nextFloat() * getHeight() : 100.0f;
    
    auto* mass = new MassPoint();
    addAndMakeVisible (mass);
    mass->setCentrePosition (juce::Point<int>(static_cast<int>(x), static_cast<int>(y)));
    
    mass->onMassDropped = [this]() { repaint(); };
    mass->onMassMoved = [this, mass]() 
    { 
        int index = massPoints.indexOf (mass);
        if (index >= 0 && static_cast<size_t>(index) < audioProcessor.getMassPoints().size())
        {
            auto center = mass->getBounds().getCentre();
            audioProcessor.updateMassPoint (index, juce::Point<float>(center.x, center.y), mass->getMassMultiplier());
        }
        repaint(); 
    };
    mass->onDeleteRequested = [this, mass]() 
    {
        int index = massPoints.indexOf (mass);
        if (index >= 0)
            audioProcessor.removeMassPoint (index);
        massPoints.removeObject (mass);
        repaint();
    };
    
    massPoints.add (mass);
    audioProcessor.addMassPoint (juce::Point<float>(x, y), mass->getMassMultiplier());
    
    LOG_INFO("Created mass point at (" + juce::String(x) + ", " + juce::String(y) + ")");
    repaint();
}

void Canvas::newSpawnPoint()
{
    if (spawnPoints.size() >= maxSpawnPoints)
    {
        LOG_WARNING("Maximum spawn points reached (" + juce::String(maxSpawnPoints) + ")");
        return;
    }
    
    juce::Random random;
    float x = getWidth() > 0 ? random.nextFloat() * getWidth() : 100.0f;
    float y = getHeight() > 0 ? random.nextFloat() * getHeight() : 100.0f;
    
    auto* spawn = new SpawnPoint();
    addAndMakeVisible (spawn);
    spawn->setCentrePosition (juce::Point<int>(static_cast<int>(x), static_cast<int>(y)));
    
    spawn->onSpawnPointMoved = [this, spawn]() 
    { 
        int index = spawnPoints.indexOf (spawn);
        if (index >= 0 && static_cast<size_t>(index) < audioProcessor.getSpawnPoints().size())
        {
            auto center = spawn->getBounds().getCentre();
            auto momentum = spawn->getMomentumVector();
            float angle = std::atan2(momentum.y, momentum.x);
            audioProcessor.updateSpawnPoint (index, juce::Point<float>(center.x, center.y), angle);
        }
        repaint(); 
    };
    spawn->onSelectionChanged = [this]() { repaint(); };
    spawn->onDeleteRequested = [this, spawn]() 
    {
        int index = spawnPoints.indexOf (spawn);
        if (index >= 0)
            audioProcessor.removeSpawnPoint (index);
        spawnPoints.removeObject (spawn);
        repaint();
    };
    spawn->getSpawnPointCount = [this]() 
    {
        return spawnPoints.size();
    };
    
    spawnPoints.add (spawn);
    
    auto momentum = spawn->getMomentumVector();
    float angle = std::atan2(momentum.y, momentum.x);
    audioProcessor.addSpawnPoint (juce::Point<float>(x, y), angle);
    
    LOG_INFO("Created spawn point at (" + juce::String(x) + ", " + juce::String(y) + ")");
    repaint();
}

void Canvas::drawGravityWaves (juce::Graphics& g)
{
    const auto& massPointsData = audioProcessor.getMassPoints();
    
    if (massPointsData.empty())
        return;
    
    // Draw interference patterns when multiple mass points exist
    if (massPoints.size() >= 2)
    {
        int gridSize = 20;
        float cellWidth = getWidth() / (float)gridSize;
        float cellHeight = getHeight() / (float)gridSize;
        
        g.setColour (juce::Colours::blue.withAlpha (0.2f));
        
        for (int x = 0; x < gridSize; ++x)
        {
            for (int y = 0; y < gridSize; ++y)
            {
                juce::Point<float> gridPoint (x * cellWidth + cellWidth / 2, 
                                              y * cellHeight + cellHeight / 2);
                
                float totalPotential = 0.0f;
                juce::Point<float> totalForce (0.0f, 0.0f);
                
                for (auto* mass : massPoints)
                {
                    juce::Point<float> massCenter (mass->getBounds().getCentreX(), 
                                                   mass->getBounds().getCentreY());
                    
                    float distance = gridPoint.getDistanceFrom (massCenter);
                    if (distance > 1.0f)
                    {
                        float potential = mass->getMassMultiplier() / distance;
                        totalPotential += potential;
                        
                        juce::Point<float> direction = (massCenter - gridPoint) / distance;
                        totalForce += direction * potential;
                    }
                }
                
                if (totalPotential > 0.01f)
                {
                    float forceMagnitude = totalForce.getDistanceFromOrigin();
                    if (forceMagnitude > 0.0f)
                    {
                        float lineLength = juce::jmin (forceMagnitude * 10.0f, 15.0f);
                        juce::Point<float> normalizedForce = totalForce / forceMagnitude;
                        juce::Point<float> endPoint = gridPoint + normalizedForce * lineLength;
                        
                        g.setColour (juce::Colours::darkblue.withAlpha (juce::jmin (totalPotential, 0.4f)));
                        g.drawLine (gridPoint.x, gridPoint.y, endPoint.x, endPoint.y, 1.0f);
                    }
                }
            }
        }
    }
}

void Canvas::drawMomentumArrows (juce::Graphics& g)
{
    for (auto* spawn : spawnPoints)
    {
        if (spawn->isSelected())
        {
            juce::Point<float> center = getSpawnPointCenter (spawn);
            juce::Point<float> momentumVector = spawn->getMomentumVector();
            juce::Point<float> arrowEnd = center + momentumVector;
            
            // Gradient from semi-transparent at base to solid at tip
            juce::ColourGradient gradient (
                juce::Colours::red.withAlpha(0.3f),
                center.x, center.y,
                juce::Colours::red,
                arrowEnd.x, arrowEnd.y,
                false
            );
            g.setGradientFill (gradient);
            g.drawArrow (juce::Line<float> (center, arrowEnd), 3.0f, 12.0f, 10.0f);
        }
    }
}

//==============================================================================
void Canvas::mouseDown (const juce::MouseEvent& event)
{
    juce::Point<float> mousePos = event.position;
    
    // Right-click shows context menu
    if (event.mods.isPopupMenu())
    {
        juce::PopupMenu menu;
        menu.setLookAndFeel (&popupMenuLookAndFeel);
        
        bool canAddMass = massPoints.size() < maxMassPoints;
        bool canAddSpawn = spawnPoints.size() < maxSpawnPoints;
        
        menu.addItem (1, "mass", canAddMass);
        menu.addItem (2, "emitter", canAddSpawn);
        
        auto options = juce::PopupMenu::Options()
            .withTargetScreenArea (juce::Rectangle<int> (event.getScreenPosition().x, 
                                                         event.getScreenPosition().y, 1, 1))
            .withParentComponent (this);
        
        menu.showMenuAsync (options,
                           [this, mousePos] (int result)
                           {
                               if (result == 1 && massPoints.size() < maxMassPoints)
                               {
                                   audioProcessor.addMassPoint (mousePos, 2.0f);
                                   
                                   auto* mass = new MassPoint();
                                   addAndMakeVisible (mass);
                                   mass->setCentrePosition (mousePos.toInt());
                                   
                                   mass->onMassDropped = [this]() { repaint(); };
                                   mass->onMassMoved = [this, mass]() 
                                   { 
                                       int index = massPoints.indexOf (mass);
                                       if (index >= 0 && static_cast<size_t>(index) < audioProcessor.getMassPoints().size())
                                       {
                                           auto center = mass->getBounds().getCentre();
                                           audioProcessor.updateMassPoint (index, juce::Point<float>(center.x, center.y), mass->getMassMultiplier());
                                       }
                                       repaint(); 
                                   };
                                   mass->onDeleteRequested = [this, mass]() 
                                   {
                                       int index = massPoints.indexOf (mass);
                                       if (index >= 0)
                                           audioProcessor.removeMassPoint (index);
                                       massPoints.removeObject (mass);
                                       repaint();
                                   };
                                   
                                   massPoints.add (mass);
                                   LOG_INFO("Added mass point at (" + juce::String(mousePos.x) + ", " + juce::String(mousePos.y) + ")");
                               }
                               else if (result == 2 && spawnPoints.size() < maxSpawnPoints)
                               {
                                   audioProcessor.addSpawnPoint (mousePos, 0.0f);
                                   
                                   auto* spawn = new SpawnPoint();
                                   addAndMakeVisible (spawn);
                                   spawn->setCentrePosition (mousePos.toInt());
                                   
                                   spawn->onSpawnPointMoved = [this, spawn]() 
                                   { 
                                       int index = spawnPoints.indexOf (spawn);
                                       if (index >= 0 && static_cast<size_t>(index) < audioProcessor.getSpawnPoints().size())
                                       {
                                           auto center = spawn->getBounds().getCentre();
                                           auto momentum = spawn->getMomentumVector();
                                           float angle = std::atan2(momentum.y, momentum.x);
                                           audioProcessor.updateSpawnPoint (index, juce::Point<float>(center.x, center.y), angle);
                                       }
                                       repaint(); 
                                   };
                                   spawn->onSelectionChanged = [this]() { repaint(); };
                                   spawn->onDeleteRequested = [this, spawn]() 
                                   {
                                       int index = spawnPoints.indexOf (spawn);
                                       if (index >= 0)
                                           audioProcessor.removeSpawnPoint (index);
                                       spawnPoints.removeObject (spawn);
                                       repaint();
                                   };
                                   spawn->getSpawnPointCount = [this]() 
                                   {
                                       return spawnPoints.size();
                                   };
                                   
                                   spawnPoints.add (spawn);
                                   LOG_INFO("Added spawn point at (" + juce::String(mousePos.x) + ", " + juce::String(mousePos.y) + ")");
                               }
                           });
        return;
    }
    
    // Check if clicking near arrow tip of selected spawn point
    for (auto* spawn : spawnPoints)
    {
        if (spawn->isSelected() && isMouseNearArrowTip (spawn, mousePos))
        {
            draggedArrowSpawnPoint = spawn;
            setMouseCursor (juce::MouseCursor::CrosshairCursor);
            return;
        }
    }
    
    // Check if clicking on a spawn point or mass point
    bool clickedOnComponent = false;
    
    for (int i = 0; i < spawnPoints.size(); ++i)
    {
        if (spawnPoints[i]->getBounds().contains (mousePos.toInt()))
        {
            clickedOnComponent = true;
            break;
        }
    }
    
    if (!clickedOnComponent)
    {
        for (int i = 0; i < massPoints.size(); ++i)
        {
            if (massPoints[i]->getBounds().contains (mousePos.toInt()))
            {
                clickedOnComponent = true;
                break;
            }
        }
    }
    
    // Clicking on empty canvas deselects all spawn points
    if (!clickedOnComponent)
    {
        for (auto* spawn : spawnPoints)
        {
            spawn->setSelected (false);
        }
        repaint();
    }
    
    draggedArrowSpawnPoint = nullptr;
}

void Canvas::mouseDrag (const juce::MouseEvent& event)
{
    if (draggedArrowSpawnPoint != nullptr)
    {
        juce::Point<float> center = getSpawnPointCenter (draggedArrowSpawnPoint);
        juce::Point<float> mousePos = event.position;
        juce::Point<float> newVector = mousePos - center;
        
        // Constrain to min and max length
        float length = newVector.getDistanceFromOrigin();
        
        if (length < minArrowLength)
        {
            if (length > 0.0f)
                newVector = (newVector / length) * minArrowLength;
            else
                newVector = juce::Point<float> (minArrowLength, 0.0f);
        }
        else if (length > maxArrowLength)
        {
            newVector = (newVector / length) * maxArrowLength;
        }
        
        draggedArrowSpawnPoint->setMomentumVector (newVector);
        repaint();
    }
}

void Canvas::mouseUp (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    
    if (draggedArrowSpawnPoint != nullptr)
    {
        // Update processor with new momentum angle
        int index = spawnPoints.indexOf (draggedArrowSpawnPoint);
        if (index >= 0 && static_cast<size_t>(index) < audioProcessor.getSpawnPoints().size())
        {
            auto center = draggedArrowSpawnPoint->getBounds().getCentre();
            auto vector = draggedArrowSpawnPoint->getMomentumVector();
            float angle = std::atan2(vector.y, vector.x);
            audioProcessor.updateSpawnPoint (index, juce::Point<float>(center.x, center.y), angle);
        }
        
        draggedArrowSpawnPoint = nullptr;
        setMouseCursor (juce::MouseCursor::NormalCursor);
    }
}

//==============================================================================
bool Canvas::isMouseNearArrowTip (SpawnPoint* spawn, const juce::Point<float>& mousePos) const
{
    juce::Point<float> center = getSpawnPointCenter (spawn);
    juce::Point<float> arrowTip = center + spawn->getMomentumVector();
    return mousePos.getDistanceFrom (arrowTip) < 8.0f;
}

juce::Point<float> Canvas::getSpawnPointCenter (SpawnPoint* spawn) const
{
    return juce::Point<float> (spawn->getBounds().getCentreX(), 
                              spawn->getBounds().getCentreY());
}

//==============================================================================
void Canvas::spawnParticle()
{
    if (spawnPoints.isEmpty())
    {
        LOG_WARNING("Cannot spawn particle - no spawn points");
        return;
    }
    
    auto* processorParticles = audioProcessor.getParticles();
    auto& particlesLock = audioProcessor.getParticlesLock();
    const juce::ScopedLock lock (particlesLock);
    
    if (processorParticles->size() >= maxParticles)
    {
        processorParticles->remove (0);
    }
    
    // Round-robin through spawn points
    auto* spawn = spawnPoints[nextSpawnPointIndex];
    nextSpawnPointIndex = (nextSpawnPointIndex + 1) % spawnPoints.size();
    
    juce::Point<float> spawnPos = getSpawnPointCenter (spawn);
    juce::Point<float> initialVelocity = spawn->getMomentumVector() * 2.0f;
    
    // Manual spawns: MIDI note -1, default ADSR, velocity/pitch 1.0
    audioProcessor.spawnParticle (spawnPos, initialVelocity, 1.0f, 1.0f, -1, 0.01f, 0.1f, 0.7f, 0.5f);
    
    repaint();
}

void Canvas::spawnParticleFromMidi (int midiNote, float midiVelocity)
{
    if (spawnPoints.isEmpty())
    {
        LOG_WARNING("Cannot spawn particle - no spawn points");
        return;
    }
    
    auto* processorParticles = audioProcessor.getParticles();
    auto& particlesLock = audioProcessor.getParticlesLock();
    const juce::ScopedLock lock (particlesLock);
    
    if (processorParticles->size() >= maxParticles)
    {
        processorParticles->remove (0);
    }
    
    // Round-robin through spawn points
    auto* spawn = spawnPoints[nextSpawnPointIndex];
    nextSpawnPointIndex = (nextSpawnPointIndex + 1) % spawnPoints.size();
    
    juce::Point<float> spawnPos = getSpawnPointCenter (spawn);
    juce::Point<float> initialVelocity = spawn->getMomentumVector() * 2.0f;
    
    // Calculate pitch shift from MIDI note (C3 = 60 = no shift)
    float semitoneOffset = midiNote - 60;
    float pitchShift = std::pow (2.0f, semitoneOffset / 12.0f);
    
    // Get ADSR from processor
    auto* attack = audioProcessor.getAPVTS().getRawParameterValue ("attack");
    auto* sustain = audioProcessor.getAPVTS().getRawParameterValue ("sustain");
    auto* release = audioProcessor.getAPVTS().getRawParameterValue ("release");
    float attackTime = attack ? attack->load() : 0.01f;
    float sustainLevelLinear = sustain ? sustain->load() : 0.7f;
    float releaseTime = release ? release->load() : 0.5f;
    
    // Convert sustain from linear (0-1) to logarithmic (-60dB to 0dB)
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
    
    audioProcessor.spawnParticle (spawnPos, initialVelocity, midiVelocity, pitchShift, midiNote, attackTime, sustainLevel, sustainLevelLinear, releaseTime);
    
    repaint();
}

void Canvas::timerCallback()
{
    const float deltaTime = 1.0f / 60.0f;
    
    
    for (auto* spawn : spawnPoints)
    {
        spawn->updateRotation (deltaTime);
        spawn->repaint();
    }
    
    for (auto* mass : massPoints)
    {
        mass->updateRotation (deltaTime);
        mass->repaint();
    }
    
    repaint();
}

void Canvas::drawParticles (juce::Graphics& g)
{
    auto* processorParticles = audioProcessor.getParticles();
    auto& particlesLock = audioProcessor.getParticlesLock();
    const juce::ScopedLock lock (particlesLock);
    
    for (auto* particle : *processorParticles)
    {
        particle->draw (g);
    }
}

void Canvas::drawWaveform (juce::Graphics& g)
{
    if (audioBuffer == nullptr || audioBuffer->getNumSamples() == 0)
        return;
    
    const int canvasHeight = getHeight();
    const int canvasWidth = getWidth();
    
    // Waveform opacity varies based on particle proximity
    const float minOpacity = 0.1f;
    const float maxOpacity = 1.0f;
    const float influenceRadius = 30.0f;
    
    for (int y = 0; y < canvasHeight; y += 4)
    {
        // Map y to sample (bottom = start, top = end)
        float normalizedPosition = 1.0f - (static_cast<float>(y) / canvasHeight);
        int sampleIndex = static_cast<int>(normalizedPosition * audioBuffer->getNumSamples());
        sampleIndex = juce::jlimit (0, audioBuffer->getNumSamples() - 1, sampleIndex);
        
        // Average magnitude across channels
        float magnitude = 0.0f;
        for (int channel = 0; channel < audioBuffer->getNumChannels(); ++channel)
        {
            magnitude += std::abs (audioBuffer->getSample (channel, sampleIndex));
        }
        magnitude /= audioBuffer->getNumChannels();
        
        float lineHalfWidth = magnitude * (canvasWidth * 0.4f);
        float centerX = canvasWidth / 2.0f;
        
        if (lineHalfWidth < 0.5f)
            continue;
            
        // Calculate opacity based on nearby particles
        float maxLeftInfluence = 0.0f;
        float maxRightInfluence = 0.0f;
        float maxCenterInfluence = 0.0f;
        
        auto* processorParticles = audioProcessor.getParticles();
        auto& particlesLock = audioProcessor.getParticlesLock();
        const juce::ScopedLock lock (particlesLock);
        
        for (auto* particle : *processorParticles)
        {
            if (particle == nullptr)
                continue;
                
            auto pos = particle->getPosition();
            float distance = std::abs(pos.y - y);
            
            if (distance < influenceRadius)
            {
                float influence = 1.0f - (distance / influenceRadius);
                influence = influence * influence;
                influence *= particle->getADSRAmplitude();
                
                float normalizedX = pos.x / canvasWidth;
                
                if (normalizedX < 0.5f)
                {
                    float leftSidedness = (0.5f - normalizedX) * 2.0f;
                    maxLeftInfluence = juce::jmax(maxLeftInfluence, influence * leftSidedness);
                    maxCenterInfluence = juce::jmax(maxCenterInfluence, influence * (1.0f - leftSidedness));
                }
                else
                {
                    float rightSidedness = (normalizedX - 0.5f) * 2.0f;
                    maxRightInfluence = juce::jmax(maxRightInfluence, influence * rightSidedness);
                    maxCenterInfluence = juce::jmax(maxCenterInfluence, influence * (1.0f - rightSidedness));
                }
            }
        }
        
        // Calculate final opacities
        float leftOpacity = minOpacity + maxLeftInfluence * (maxOpacity - minOpacity) + maxCenterInfluence * (maxOpacity - minOpacity) * 0.5f;
        float rightOpacity = minOpacity + maxRightInfluence * (maxOpacity - minOpacity) + maxCenterInfluence * (maxOpacity - minOpacity) * 0.5f;
        
        leftOpacity = juce::jlimit(minOpacity, maxOpacity, leftOpacity);
        rightOpacity = juce::jlimit(minOpacity, maxOpacity, rightOpacity);
        
        juce::ColourGradient gradient(
            juce::Colour(0xFF, 0xFF, 0xF2).withAlpha(leftOpacity),
            centerX - lineHalfWidth, static_cast<float>(y),
            juce::Colour(0xFF, 0xFF, 0xF2).withAlpha(rightOpacity),
            centerX + lineHalfWidth, static_cast<float>(y),
            false
        );
        
        g.setGradientFill(gradient);
        g.drawLine(centerX - lineHalfWidth, static_cast<float>(y),
                   centerX + lineHalfWidth, static_cast<float>(y),
                   1.0f);
    }
}

void Canvas::setAudioBuffer (const juce::AudioBuffer<float>* buffer)
{
    audioBuffer = buffer;
    repaint();
}

//==============================================================================
bool Canvas::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& filePath : files)
    {
        juce::File file (filePath);
        juce::String extension = file.getFileExtension().toLowerCase();
        
        if (extension == ".wav" || extension == ".mp3" || 
            extension == ".aiff" || extension == ".aif" ||
            extension == ".flac" || extension == ".ogg" ||
            extension == ".m4a")
        {
            return true;
        }
    }
    
    return false;
}

void Canvas::fileDragEnter (const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused (x, y, files);
    isDraggingFile = true;
    repaint();
}

void Canvas::fileDragExit (const juce::StringArray& files)
{
    juce::ignoreUnused (files);
    isDraggingFile = false;
    repaint();
}

void Canvas::filesDropped (const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused (x, y);
    isDraggingFile = false;
    
    for (const auto& filePath : files)
    {
        juce::File file (filePath);
        juce::String extension = file.getFileExtension().toLowerCase();
        
        if (extension == ".wav" || extension == ".mp3" || 
            extension == ".aiff" || extension == ".aif" ||
            extension == ".flac" || extension == ".ogg" ||
            extension == ".m4a")
        {
            LOG_INFO("Audio file dropped: " + file.getFullPathName());
            
            if (onAudioFileLoaded)
                onAudioFileLoaded (file);
            
            repaint();
            return;
        }
    }
    
    LOG_WARNING("No valid audio file in dropped files");
    repaint();
}

//==============================================================================
// Deprecated - components render themselves
void Canvas::drawSpawnPoints (juce::Graphics& g)
{
    juce::ignoreUnused (g);
}

void Canvas::drawMassPoints (juce::Graphics& g)
{
    juce::ignoreUnused (g);
}

//==============================================================================
void Canvas::syncGuiFromProcessor()
{
    spawnPoints.clear();
    massPoints.clear();
    
    // Recreate mass point GUI components from processor data
    const auto& massPointsData = audioProcessor.getMassPoints();
    for (size_t i = 0; i < massPointsData.size(); ++i)
    {
        const auto& mpData = massPointsData[i];
        
        auto* mass = new MassPoint();
        addAndMakeVisible (mass);
        mass->setCentrePosition (mpData.position.toInt());
        mass->setRadius (static_cast<int>(50.0f * mpData.massMultiplier));
        
        mass->onMassDropped = [this]() { repaint(); };
        mass->onMassMoved = [this, mass]() 
        { 
            int index = massPoints.indexOf (mass);
            if (index >= 0 && static_cast<size_t>(index) < audioProcessor.getMassPoints().size())
            {
                auto center = mass->getBounds().getCentre();
                audioProcessor.updateMassPoint (index, juce::Point<float>(center.x, center.y), mass->getMassMultiplier());
            }
            repaint(); 
        };
        mass->onDeleteRequested = [this, mass]() 
        {
            int index = massPoints.indexOf (mass);
            if (index >= 0)
                audioProcessor.removeMassPoint (index);
            massPoints.removeObject (mass);
            repaint();
        };
        
        massPoints.add (mass);
    }
    
    // Recreate spawn point GUI components from processor data
    const auto& spawnPointsData = audioProcessor.getSpawnPoints();
    for (size_t i = 0; i < spawnPointsData.size(); ++i)
    {
        const auto& spData = spawnPointsData[i];
        
        auto* spawn = new SpawnPoint();
        addAndMakeVisible (spawn);
        spawn->setCentrePosition (spData.position.toInt());
        
        float momentum = 20.0f;
        juce::Point<float> momentumVector (
            std::cos(spData.momentumAngle) * momentum,
            std::sin(spData.momentumAngle) * momentum
        );
        spawn->setMomentumVector (momentumVector);
        
        spawn->onSpawnPointMoved = [this, spawn]() 
        { 
            int index = spawnPoints.indexOf (spawn);
            if (index >= 0 && static_cast<size_t>(index) < audioProcessor.getSpawnPoints().size())
            {
                auto center = spawn->getBounds().getCentre();
                auto momentumVec = spawn->getMomentumVector();
                float angle = std::atan2(momentumVec.y, momentumVec.x);
                audioProcessor.updateSpawnPoint (index, juce::Point<float>(center.x, center.y), angle);
            }
            repaint(); 
        };
        spawn->onSelectionChanged = [this]() { repaint(); };
        spawn->onDeleteRequested = [this, spawn]() 
        {
            int index = spawnPoints.indexOf (spawn);
            if (index >= 0)
                audioProcessor.removeSpawnPoint (index);
            spawnPoints.removeObject (spawn);
            repaint();
        };
        spawn->getSpawnPointCount = [this]() 
        {
            return spawnPoints.size();
        };
        
        spawnPoints.add (spawn);
    }
    
    LOG_INFO("Synced GUI: " + juce::String(massPoints.size()) + " mass points, " + juce::String(spawnPoints.size()) + " spawn points");
}

