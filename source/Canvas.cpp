#include "Canvas.h"
#include "Logger.h"
#include "PluginProcessor.h"

//==============================================================================
Canvas::Canvas (PluginProcessor& processor)
    : audioProcessor (processor)
{
    // Constructor
    LOG_INFO("Canvas created");
    setSize (400, 400);
    
    // Set initial canvas bounds in processor so particles know where to wrap
    audioProcessor.setCanvasBounds (getLocalBounds().toFloat());
    
    // Set gravity strength in processor
    audioProcessor.setGravityStrength (gravityStrength);
    
    // Create GUI components to match whatever is in the processor
    // (The processor may have loaded saved state, or may be empty)
    syncGuiFromProcessor();
    
    // Start timer for GUI updates (60 FPS) - physics now in processor
    startTimer (1000 / 60);
}

Canvas::~Canvas()
{
    // Destructor
    stopTimer();
    
    auto* processorParticles = audioProcessor.getParticles();
    int particleCount = processorParticles ? processorParticles->size() : 0;
    
    LOG_INFO("Canvas destroyed - had " + juce::String(spawnPoints.size()) + 
             " spawn points, " + juce::String(massPoints.size()) + " mass points, and " +
             juce::String(particleCount) + " particles");
}

//==============================================================================
// Particle Access Methods (forwarding to processor)

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
    LOG_INFO("Bounce mode " + juce::String(enabled ? "enabled" : "disabled") + " - particles will " + juce::String(enabled ? "bounce off walls" : "wrap around edges"));
}

//==============================================================================
void Canvas::paint (juce::Graphics& g)
{
    // Make canvas transparent so the background image shows through
    // (Don't fill with any color)
    
    // Show "drop" text if dragging valid audio file
    if (isDraggingFile && customTypeface != nullptr)
    {
        g.setColour (juce::Colour (0xFF, 0xFF, 0xF2).withAlpha (0.4f)); // Same style as slider values
        auto font = juce::Font (juce::FontOptions (customTypeface).withHeight (80.0f)); // Same size as slider values
        g.setFont (font);
        
        // Draw "drop" text in center of canvas
        g.drawText ("drop", 
                   juce::Rectangle<float>(0.0f, 0.0f, (float)getWidth(), (float)getHeight()), 
                   juce::Justification::centred, true);
    }
    
    // Draw gravity waves first (behind everything)
    if (showGravityWaves)
        drawGravityWaves (g);
    
    // Draw waveform (above gravity waves, behind particles)
    drawWaveform (g);
    
    // Draw particles (above gravity waves)
    drawParticles (g);
    
    // SpawnPoint and MassPoint components render themselves now
    // (they are child components that call their own paint() methods)
    
    // Draw momentum arrows (on top of gravity waves, behind spawn points)
    drawMomentumArrows (g);
}

void Canvas::resized()
{
    // Update canvas bounds in processor whenever canvas is resized
    audioProcessor.setCanvasBounds (getLocalBounds().toFloat());
    
    LOG_INFO("Canvas resized to " + juce::String(getWidth()) + "x" + juce::String(getHeight()));
}

void Canvas::newMassPoint()
{
    // Check limit
    if (massPoints.size() >= maxMassPoints)
    {
        LOG_WARNING("Maximum mass points reached (" + juce::String(maxMassPoints) + ")");
        return;
    }
    
    // Place at random position on canvas
    juce::Random random;
    float x = getWidth() > 0 ? random.nextFloat() * getWidth() : 100.0f;
    float y = getHeight() > 0 ? random.nextFloat() * getHeight() : 100.0f;
    
    // Create GUI component for interaction and custom rendering
    auto* mass = new MassPoint();
    addAndMakeVisible (mass);
    mass->setCentrePosition (juce::Point<int>(static_cast<int>(x), static_cast<int>(y)));
    
    // Set up callbacks to sync with processor
    mass->onMassDropped = [this]() { repaint(); };
    mass->onMassMoved = [this, mass]() 
    { 
        // Update processor data when GUI component moves
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
        // Remove from both GUI and processor
        int index = massPoints.indexOf (mass);
        if (index >= 0)
            audioProcessor.removeMassPoint (index);
        massPoints.removeObject (mass);
        repaint();
        LOG_INFO("Deleted mass point");
    };
    
    massPoints.add (mass);
    
    // Add to processor
    juce::Point<float> pos (x, y);
    audioProcessor.addMassPoint (pos, mass->getMassMultiplier());
    
    LOG_INFO("Created new mass point at (" + juce::String(x) + ", " + juce::String(y) + ")");
    repaint();
}

void Canvas::newSpawnPoint()
{
    // Check limit
    if (spawnPoints.size() >= maxSpawnPoints)
    {
        LOG_WARNING("Maximum spawn points reached (" + juce::String(maxSpawnPoints) + ")");
        return;
    }
    
    // Place at random position on canvas
    juce::Random random;
    float x = getWidth() > 0 ? random.nextFloat() * getWidth() : 100.0f;
    float y = getHeight() > 0 ? random.nextFloat() * getHeight() : 100.0f;
    
    // Create GUI component for interaction and custom rendering
    auto* spawn = new SpawnPoint();
    addAndMakeVisible (spawn);
    spawn->setCentrePosition (juce::Point<int>(static_cast<int>(x), static_cast<int>(y)));
    
    // Set up callbacks to sync with processor
    spawn->onSpawnPointMoved = [this, spawn]() 
    { 
        // Update processor data when GUI component moves
        int index = spawnPoints.indexOf (spawn);
        if (index >= 0 && static_cast<size_t>(index) < audioProcessor.getSpawnPoints().size())
        {
            auto center = spawn->getBounds().getCentre();
            // Get the momentum vector angle
            auto momentum = spawn->getMomentumVector();
            float angle = std::atan2(momentum.y, momentum.x);
            audioProcessor.updateSpawnPoint (index, juce::Point<float>(center.x, center.y), angle);
        }
        repaint(); 
    };
    spawn->onSelectionChanged = [this]() { repaint(); };
    spawn->onDeleteRequested = [this, spawn]() 
    {
        // Remove from both GUI and processor
        int index = spawnPoints.indexOf (spawn);
        if (index >= 0)
            audioProcessor.removeSpawnPoint (index);
        spawnPoints.removeObject (spawn);
        repaint();
        LOG_INFO("Deleted spawn point");
    };
    spawn->getSpawnPointCount = [this]() 
    {
        return spawnPoints.size();
    };
    
    spawnPoints.add (spawn);
    
    // Add to processor with momentum angle
    juce::Point<float> pos (x, y);
    auto momentum = spawn->getMomentumVector();
    float angle = std::atan2(momentum.y, momentum.x);
    audioProcessor.addSpawnPoint (pos, angle);
    
    LOG_INFO("Created new spawn point at (" + juce::String(x) + ", " + juce::String(y) + ") angle: " + juce::String(angle));
    repaint();
}

void Canvas::drawGravityWaves (juce::Graphics& g)
{
    const auto& massPointsData = audioProcessor.getMassPoints();
    
    if (massPointsData.empty())
        return;
    
    // Ripples around mass points removed - vortex images are now used instead
    // (Keeping the interference pattern visualization below)
    
    // If there are multiple mass points, draw interference patterns
    if (massPoints.size() >= 2)
    {
        // Sample points on a grid to calculate gravitational field
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
                
                // Calculate combined gravitational potential at this point
                float totalPotential = 0.0f;
                juce::Point<float> totalForce (0.0f, 0.0f);
                
                for (auto* mass : massPoints)
                {
                    juce::Point<float> massCenter (mass->getBounds().getCentreX(), 
                                                   mass->getBounds().getCentreY());
                    
                    float distance = gridPoint.getDistanceFrom (massCenter);
                    if (distance > 1.0f)
                    {
                        // Apply mass multiplier to potential
                        float potential = mass->getMassMultiplier() / distance;
                        totalPotential += potential;
                        
                        // Calculate force direction
                        juce::Point<float> direction = (massCenter - gridPoint) / distance;
                        totalForce += direction * potential;
                    }
                }
                
                // Draw field lines based on potential
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
    // Draw momentum arrows for selected spawn points
    for (auto* spawn : spawnPoints)
    {
        if (spawn->isSelected())
        {
            // Get spawn point center
            juce::Point<float> center = getSpawnPointCenter (spawn);
            juce::Point<float> momentumVector = spawn->getMomentumVector();
            
            // Draw arrow from center to momentum vector endpoint with red gradient
            juce::Point<float> arrowEnd = center + momentumVector;
            
            // Create gradient from base (semi-transparent red) to tip (solid red)
            juce::ColourGradient gradient (
                juce::Colours::red.withAlpha(0.3f),  // Base: 30% opacity
                center.x, center.y,
                juce::Colours::red,                   // Tip: 100% opacity
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
    
    LOG_INFO("Canvas::mouseDown - Position: (" + juce::String(mousePos.x, 2) + ", " + 
             juce::String(mousePos.y, 2) + "), Button: " + 
             (event.mods.isLeftButtonDown() ? "Left" : 
              event.mods.isRightButtonDown() ? "Right" : "Other") +
             ", Popup menu: " + (event.mods.isPopupMenu() ? "Yes" : "No"));
    
    // Right-click shows context menu to add mass or spawn point
    if (event.mods.isPopupMenu())
    {
        LOG_INFO("Canvas::mouseDown - Showing popup menu");
        
        juce::PopupMenu menu;
        menu.setLookAndFeel (&popupMenuLookAndFeel);
        
        // Check limits
        bool canAddMass = massPoints.size() < maxMassPoints;
        bool canAddSpawn = spawnPoints.size() < maxSpawnPoints;
        
        LOG_INFO("Canvas::mouseDown - Menu options - Can add mass: " + 
                 juce::String(canAddMass ? "Yes" : "No") + ", Can add spawn: " + 
                 juce::String(canAddSpawn ? "Yes" : "No"));
        
        // Add menu items (greyed out if at limit)
        menu.addItem (1, "mass", canAddMass);
        menu.addItem (2, "emitter", canAddSpawn);
        
        auto options = juce::PopupMenu::Options()
            .withTargetScreenArea (juce::Rectangle<int> (event.getScreenPosition().x, 
                                                         event.getScreenPosition().y, 1, 1))
            .withParentComponent (this);
        
        menu.showMenuAsync (options,
                           [this, mousePos] (int result)
                           {
                               LOG_INFO("Canvas::mouseDown - Menu result: " + juce::String(result));
                               
                               if (result == 1 && massPoints.size() < maxMassPoints)
                               {
                                   LOG_INFO("Canvas - Adding mass point at (" + 
                                            juce::String(mousePos.x, 2) + ", " + 
                                            juce::String(mousePos.y, 2) + ")");
                                   
                                   // Add mass point to processor first
                                   audioProcessor.addMassPoint (mousePos, 2.0f); // Default to medium mass
                                   
                                   // Create GUI component to match
                                   auto* mass = new MassPoint();
                                   addAndMakeVisible (mass);
                                   mass->setCentrePosition (mousePos.toInt());
                                   
                                   // Set up callbacks to sync with processor
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
                                       LOG_INFO("Deleted mass point");
                                   };
                                   
                                   massPoints.add (mass);
                                   LOG_INFO("Added mass point at (" + juce::String(mousePos.x) + ", " + juce::String(mousePos.y) + ") - Total: " + juce::String(massPoints.size()));
                               }
                               else if (result == 2 && spawnPoints.size() < maxSpawnPoints)
                               {
                                   LOG_INFO("Canvas - Adding spawn point at (" + 
                                            juce::String(mousePos.x, 2) + ", " + 
                                            juce::String(mousePos.y, 2) + ")");
                                   
                                   // Add spawn point to processor first
                                   audioProcessor.addSpawnPoint (mousePos, 0.0f); // Default angle pointing right
                                   
                                   // Create GUI component to match
                                   auto* spawn = new SpawnPoint();
                                   addAndMakeVisible (spawn);
                                   spawn->setCentrePosition (mousePos.toInt());
                                   
                                   // Set up callbacks to sync with processor
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
                                       LOG_INFO("Deleted spawn point");
                                   };
                                   spawn->getSpawnPointCount = [this]() 
                                   {
                                       return spawnPoints.size();
                                   };
                                   
                                   spawnPoints.add (spawn);
                                   LOG_INFO("Added spawn point at (" + juce::String(mousePos.x) + ", " + juce::String(mousePos.y) + ") - Total: " + juce::String(spawnPoints.size()));
                               }
                           });
        return;
    }
    
    // Check if clicking near any arrow tip (only for selected spawn points)
    for (auto* spawn : spawnPoints)
    {
        if (spawn->isSelected() && isMouseNearArrowTip (spawn, mousePos))
        {
            LOG_INFO("Canvas::mouseDown - Clicked near arrow tip of selected spawn point");
            draggedArrowSpawnPoint = spawn;
            setMouseCursor (juce::MouseCursor::CrosshairCursor);
            LOG_INFO("Started dragging arrow for spawn point");
            return;
        }
    }
    
    // Check if clicking on a spawn point or mass point
    bool clickedOnComponent = false;
    int clickedSpawnIndex = -1;
    int clickedMassIndex = -1;
    
    for (int i = 0; i < spawnPoints.size(); ++i)
    {
        if (spawnPoints[i]->getBounds().contains (mousePos.toInt()))
        {
            clickedOnComponent = true;
            clickedSpawnIndex = i;
            LOG_INFO("Canvas::mouseDown - Clicked on spawn point #" + juce::String(i) + 
                     " at bounds: " + spawnPoints[i]->getBounds().toString());
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
                clickedMassIndex = i;
                LOG_INFO("Canvas::mouseDown - Clicked on mass point #" + juce::String(i) + 
                         " at bounds: " + massPoints[i]->getBounds().toString());
                break;
            }
        }
    }
    
    // If clicking on empty canvas (not on any component), deselect all spawn points
    if (!clickedOnComponent)
    {
        LOG_INFO("Canvas::mouseDown - Clicked on empty canvas, deselecting all spawn points");
        for (auto* spawn : spawnPoints)
        {
            spawn->setSelected (false);
        }
        repaint(); // Force repaint to hide arrows immediately
    }
    else
    {
        LOG_INFO("Canvas::mouseDown - Clicked on component (spawn: " + 
                 juce::String(clickedSpawnIndex) + ", mass: " + 
                 juce::String(clickedMassIndex) + ")");
    }
    
    // If not clicking on arrow, allow spawn points to handle their own dragging
    draggedArrowSpawnPoint = nullptr;
}

void Canvas::mouseDrag (const juce::MouseEvent& event)
{
    if (draggedArrowSpawnPoint != nullptr)
    {
        // Update arrow vector based on mouse position relative to spawn point center
        juce::Point<float> center = getSpawnPointCenter (draggedArrowSpawnPoint);
        juce::Point<float> mousePos = event.position;
        juce::Point<float> newVector = mousePos - center;
        
        // Constrain to min and max length
        float length = newVector.getDistanceFromOrigin();
        
        LOG_INFO("Canvas::mouseDrag - Arrow drag - Mouse: (" + juce::String(mousePos.x, 2) + 
                 ", " + juce::String(mousePos.y, 2) + "), Center: (" + 
                 juce::String(center.x, 2) + ", " + juce::String(center.y, 2) + 
                 "), Vector length: " + juce::String(length, 2));
        
        if (length < minArrowLength)
        {
            if (length > 0.0f)
                newVector = (newVector / length) * minArrowLength;
            else
                newVector = juce::Point<float> (minArrowLength, 0.0f); // Default direction if length is 0
            
            LOG_INFO("Canvas::mouseDrag - Constrained to min length: " + juce::String(minArrowLength));
        }
        else if (length > maxArrowLength)
        {
            newVector = (newVector / length) * maxArrowLength;
            LOG_INFO("Canvas::mouseDrag - Constrained to max length: " + juce::String(maxArrowLength));
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
        auto vector = draggedArrowSpawnPoint->getMomentumVector();
        LOG_INFO("Stopped dragging arrow - momentum: (" + 
                 juce::String(vector.x) + ", " + juce::String(vector.y) + ")");
        
        // Update processor with new momentum angle
        int index = spawnPoints.indexOf (draggedArrowSpawnPoint);
        if (index >= 0 && static_cast<size_t>(index) < audioProcessor.getSpawnPoints().size())
        {
            auto center = draggedArrowSpawnPoint->getBounds().getCentre();
            float angle = std::atan2(vector.y, vector.x);
            audioProcessor.updateSpawnPoint (index, juce::Point<float>(center.x, center.y), angle);
            LOG_INFO("Updated processor spawn point " + juce::String(index) + " angle: " + juce::String(angle));
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
    float distance = mousePos.getDistanceFrom (arrowTip);
    return distance < 8.0f; // 8 pixel radius for easier clicking
}

juce::Point<float> Canvas::getSpawnPointCenter (SpawnPoint* spawn) const
{
    return juce::Point<float> (spawn->getBounds().getCentreX(), 
                              spawn->getBounds().getCentreY());
}

//==============================================================================
// Particle System Methods

void Canvas::spawnParticle()
{
    if (spawnPoints.isEmpty())
    {
        LOG_WARNING("Cannot spawn particle - no spawn points exist!");
        return;
    }
    
    // Get particles from processor
    auto* processorParticles = audioProcessor.getParticles();
    auto& particlesLock = audioProcessor.getParticlesLock();
    const juce::ScopedLock lock (particlesLock);
    
    // Check if we need to remove the oldest particle
    if (processorParticles->size() >= maxParticles)
    {
        // Remove the oldest particle (index 0)
        processorParticles->remove (0);
        LOG_INFO("Removed oldest particle to make room (max: " + juce::String(maxParticles) + ")");
    }
    
    // Get the spawn point using round-robin
    auto* spawn = spawnPoints[nextSpawnPointIndex];
    nextSpawnPointIndex = (nextSpawnPointIndex + 1) % spawnPoints.size();
    
    // Get spawn position and initial velocity from momentum arrow
    juce::Point<float> spawnPos = getSpawnPointCenter (spawn);
    juce::Point<float> initialVelocity = spawn->getMomentumVector() * 2.0f; // Scale up for visibility
    
    // Spawn particle in processor with manual spawn parameters
    // Manual spawns use MIDI note -1, default attack/sustain/release (0.01s, 0.7 linear/0.1 log, 0.5s), velocity 1.0, pitch 1.0
    audioProcessor.spawnParticle (spawnPos, initialVelocity, 1.0f, 1.0f, -1, 0.01f, 0.1f, 0.7f, 0.5f);
    
    LOG_INFO("Spawned particle #" + juce::String(processorParticles->size()) + 
             " at (" + juce::String(spawnPos.x) + ", " + juce::String(spawnPos.y) + 
             ") with velocity (" + juce::String(initialVelocity.x) + ", " + 
             juce::String(initialVelocity.y) + ")");
    
    repaint();
}

void Canvas::spawnParticleFromMidi (int midiNote, float midiVelocity)
{
    if (spawnPoints.isEmpty())
    {
        LOG_WARNING("Cannot spawn particle from MIDI - no spawn points exist!");
        return;
    }
    
    // Get particles from processor
    auto* processorParticles = audioProcessor.getParticles();
    auto& particlesLock = audioProcessor.getParticlesLock();
    const juce::ScopedLock lock (particlesLock);
    
    // Check if we need to remove the oldest particle
    if (processorParticles->size() >= maxParticles)
    {
        // Remove the oldest particle (index 0)
        processorParticles->remove (0);
    }
    
    // Get the spawn point using round-robin
    auto* spawn = spawnPoints[nextSpawnPointIndex];
    nextSpawnPointIndex = (nextSpawnPointIndex + 1) % spawnPoints.size();
    
    // Get spawn position and initial velocity from momentum arrow
    juce::Point<float> spawnPos = getSpawnPointCenter (spawn);
    juce::Point<float> initialVelocity = spawn->getMomentumVector() * 2.0f; // Scale up for visibility
    
    // Calculate pitch shift from MIDI note (C3 = 60 = no shift)
    // Each semitone = 2^(1/12) ratio
    float semitoneOffset = midiNote - 60; // C3 is reference
    float pitchShift = std::pow (2.0f, semitoneOffset / 12.0f);
    
    // Get current ADSR parameters from processor
    auto* attack = audioProcessor.getAPVTS().getRawParameterValue ("attack");
    auto* sustain = audioProcessor.getAPVTS().getRawParameterValue ("sustain");
    auto* release = audioProcessor.getAPVTS().getRawParameterValue ("release");
    float attackTime = attack ? attack->load() : 0.01f;
    float sustainLevelLinear = sustain ? sustain->load() : 0.7f;
    float releaseTime = release ? release->load() : 0.5f;
    
    // Convert sustain from linear to logarithmic (same as in PluginProcessor::handleNoteOn)
    float sustainLevel;
    if (sustainLevelLinear < 0.001f)
    {
        sustainLevel = 0.0f;
    }
    else
    {
        float sustainDb = (sustainLevelLinear - 1.0f) * 60.0f; // -60dB to 0dB
        sustainLevel = juce::Decibels::decibelsToGain(sustainDb);
    }
    
    // Spawn particle in processor with MIDI parameters (pass both logarithmic and linear sustain)
    audioProcessor.spawnParticle (spawnPos, initialVelocity, midiVelocity, pitchShift, midiNote, attackTime, sustainLevel, sustainLevelLinear, releaseTime);
    
    LOG_INFO("Spawned MIDI particle #" + juce::String(processorParticles->size()) + 
             " - Note: " + juce::String(midiNote) + 
             " (" + juce::MidiMessage::getMidiNoteName(midiNote, true, true, 3) + ")" +
             ", Velocity: " + juce::String(midiVelocity, 2) + 
             ", Pitch: " + juce::String(pitchShift, 3) + "x");
    
    repaint();
}

void Canvas::timerCallback()
{
    const float deltaTime = 1.0f / 60.0f; // 60 FPS
    
    // Update spawn point rotations (GUI only)
    for (auto* spawn : spawnPoints)
    {
        spawn->updateRotation (deltaTime);
        spawn->repaint();
    }
    
    // Update mass point rotations (GUI only - black holes)
    for (auto* mass : massPoints)
    {
        mass->updateRotation (deltaTime);
        mass->repaint();
    }
    
    // Repaint to show updated particle positions (particles are updated in audio thread)
    auto* processorParticles = audioProcessor.getParticles();
    if (!processorParticles->isEmpty())
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
    const float minOpacity = 0.1f;  // Base opacity when no particles
    const float maxOpacity = 1.0f;   // Max opacity when particle is at edge
    const float influenceRadius = 30.0f; // How far from a particle affects the waveform
    
    for (int y = 0; y < canvasHeight; y += 4)
    {
        // Map y position to sample position (bottom = start, top = end)
        float normalizedPosition = 1.0f - (static_cast<float>(y) / canvasHeight);
        int sampleIndex = static_cast<int>(normalizedPosition * audioBuffer->getNumSamples());
        sampleIndex = juce::jlimit (0, audioBuffer->getNumSamples() - 1, sampleIndex);
        
        // Get average magnitude across all channels at this sample
        float magnitude = 0.0f;
        for (int channel = 0; channel < audioBuffer->getNumChannels(); ++channel)
        {
            magnitude += std::abs (audioBuffer->getSample (channel, sampleIndex));
        }
        magnitude /= audioBuffer->getNumChannels();
        
        // Scale magnitude to line width (half width on each side from center)
        float lineHalfWidth = magnitude * (canvasWidth * 0.4f); // Max 40% of canvas width each side
        float centerX = canvasWidth / 2.0f;
        
        if (lineHalfWidth < 0.5f)
            continue;
            
        // Calculate opacity influence from nearby particles
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
            
            // Only consider particles near this Y line
            if (distance < influenceRadius)
            {
                // Calculate influence (1.0 when particle is on the line, 0.0 at radius)
                float influence = 1.0f - (distance / influenceRadius);
                influence = influence * influence; // Square for smoother falloff
                
                // Factor in particle's ADSR amplitude (fading particles have less effect)
                float adsrAmp = particle->getADSRAmplitude();
                influence *= adsrAmp;
                
                // Calculate particle position: 0.0 = far left, 0.5 = center, 1.0 = far right
                float normalizedX = pos.x / canvasWidth;
                
                if (normalizedX < 0.5f)
                {
                    // Particle on left side
                    float leftSidedness = (0.5f - normalizedX) * 2.0f; // 1.0 at far left, 0.0 at center
                    maxLeftInfluence = juce::jmax(maxLeftInfluence, influence * leftSidedness);
                    maxCenterInfluence = juce::jmax(maxCenterInfluence, influence * (1.0f - leftSidedness));
                }
                else
                {
                    // Particle on right side
                    float rightSidedness = (normalizedX - 0.5f) * 2.0f; // 1.0 at far right, 0.0 at center
                    maxRightInfluence = juce::jmax(maxRightInfluence, influence * rightSidedness);
                    maxCenterInfluence = juce::jmax(maxCenterInfluence, influence * (1.0f - rightSidedness));
                }
            }
        }
        
        // Calculate final opacities
        // When particle is at edge: that side = 100%, other side = 10%
        // When particle is at center: both sides = 55% (halfway between 100% and 10%)
        float leftOpacity = minOpacity + maxLeftInfluence * (maxOpacity - minOpacity) + maxCenterInfluence * (maxOpacity - minOpacity) * 0.5f;
        float rightOpacity = minOpacity + maxRightInfluence * (maxOpacity - minOpacity) + maxCenterInfluence * (maxOpacity - minOpacity) * 0.5f;
        
        // Clamp to valid range
        leftOpacity = juce::jlimit(minOpacity, maxOpacity, leftOpacity);
        rightOpacity = juce::jlimit(minOpacity, maxOpacity, rightOpacity);
        
        // Draw line with horizontal gradient from left opacity to right opacity
        juce::ColourGradient gradient(
            juce::Colour(0xFF, 0xFF, 0xF2).withAlpha(leftOpacity),  // Left color
            centerX - lineHalfWidth, static_cast<float>(y),         // Left point
            juce::Colour(0xFF, 0xFF, 0xF2).withAlpha(rightOpacity), // Right color
            centerX + lineHalfWidth, static_cast<float>(y),         // Right point
            false                                                    // Not radial
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
// File Drag and Drop

bool Canvas::isInterestedInFileDrag (const juce::StringArray& files)
{
    // Check if any file is an audio file
    for (const auto& filePath : files)
    {
        juce::File file (filePath);
        juce::String extension = file.getFileExtension().toLowerCase();
        
        // Support common audio formats
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
    LOG_INFO("Valid audio file dragged over canvas");
    repaint();
}

void Canvas::fileDragExit (const juce::StringArray& files)
{
    juce::ignoreUnused (files);
    
    isDraggingFile = false;
    LOG_INFO("File drag exited canvas");
    repaint();
}

void Canvas::filesDropped (const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused (x, y);
    
    isDraggingFile = false;
    
    // Only accept the first audio file
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
            
            // Trigger callback to load audio in processor
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
// These methods are no longer used - SpawnPoint and MassPoint components render themselves
void Canvas::drawSpawnPoints (juce::Graphics& g)
{
    juce::ignoreUnused (g);
    // SpawnPoint GUI components now render themselves via their paint() method
}

//==============================================================================
void Canvas::drawMassPoints (juce::Graphics& g)
{
    juce::ignoreUnused (g);
    // MassPoint GUI components now render themselves via their paint() method
}

//==============================================================================
// Sync GUI components with processor data
void Canvas::syncGuiFromProcessor()
{
    // Clear existing GUI components (we'll recreate them from processor data)
    spawnPoints.clear();
    massPoints.clear();
    
    // Create GUI components for each mass point in processor
    const auto& massPointsData = audioProcessor.getMassPoints();
    for (size_t i = 0; i < massPointsData.size(); ++i)
    {
        const auto& mpData = massPointsData[i];
        
        // Create GUI component
        auto* mass = new MassPoint();
        addAndMakeVisible (mass);
        mass->setCentrePosition (mpData.position.toInt());
        
        // Set mass multiplier to match processor data
        // Calculate radius from massMultiplier (radius = 50 * massMultiplier)
        int radius = static_cast<int>(50.0f * mpData.massMultiplier);
        mass->setRadius (radius);
        
        // Set up callbacks to sync with processor when GUI changes
        mass->onMassDropped = [this]() { repaint(); };
        mass->onMassMoved = [this, mass]() 
        { 
            // Update processor data when GUI component moves
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
            // Remove from both GUI and processor
            int index = massPoints.indexOf (mass);
            if (index >= 0)
                audioProcessor.removeMassPoint (index);
            massPoints.removeObject (mass);
            repaint();
            LOG_INFO("Deleted mass point");
        };
        
        massPoints.add (mass);
    }
    
    // Create GUI components for each spawn point in processor
    const auto& spawnPointsData = audioProcessor.getSpawnPoints();
    for (size_t i = 0; i < spawnPointsData.size(); ++i)
    {
        const auto& spData = spawnPointsData[i];
        
        // Create GUI component
        auto* spawn = new SpawnPoint();
        addAndMakeVisible (spawn);
        spawn->setCentrePosition (spData.position.toInt());
        
        // Set momentum vector from momentumAngle (user-set direction, not visual rotation)
        float momentum = 20.0f; // Default momentum magnitude
        juce::Point<float> momentumVector (
            std::cos(spData.momentumAngle) * momentum,
            std::sin(spData.momentumAngle) * momentum
        );
        spawn->setMomentumVector (momentumVector);
        
        // Set up callbacks to sync with processor when GUI changes
        spawn->onSpawnPointMoved = [this, spawn]() 
        { 
            // Update processor data when GUI component moves
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
            // Remove from both GUI and processor
            int index = spawnPoints.indexOf (spawn);
            if (index >= 0)
                audioProcessor.removeSpawnPoint (index);
            spawnPoints.removeObject (spawn);
            repaint();
            LOG_INFO("Deleted spawn point");
        };
        spawn->getSpawnPointCount = [this]() 
        {
            return spawnPoints.size();
        };
        
        spawnPoints.add (spawn);
    }
    
    LOG_INFO("Synced GUI from processor: " + juce::String(massPoints.size()) + 
             " mass points, " + juce::String(spawnPoints.size()) + " spawn points");
}

