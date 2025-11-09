#include "Canvas.h"
#include "Logger.h"

//==============================================================================
Canvas::Canvas()
{
    // Constructor
    LOG_INFO("Canvas created");
    setSize (400, 400);
    
    // Create the mass point (center of gravity)
    newMassPoint();
    
    // Create first spawn point
    newSpawnPoint();
    
    // Start physics timer (60 FPS)
    startTimer (1000 / 60);
}

Canvas::~Canvas()
{
    // Destructor
    stopTimer();
    LOG_INFO("Canvas destroyed - had " + juce::String(spawnPoints.size()) + 
             " spawn points, " + juce::String(massPoints.size()) + " mass points, and " +
             juce::String(particles.size()) + " particles");
}

//==============================================================================
void Canvas::paint (juce::Graphics& g)
{
    // Make canvas transparent so the background image shows through
    // (Don't fill with any color)
    
    // Draw green border if dragging valid audio file
    if (isDraggingFile)
    {
        g.setColour (juce::Colours::green);
        g.drawRect (getLocalBounds(), 3);
    }
    
    // Draw gravity waves first (behind everything)
    if (showGravityWaves)
        drawGravityWaves (g);
    
    // Draw waveform (above gravity waves, behind particles)
    drawWaveform (g);
    
    // Draw particles (above gravity waves)
    drawParticles (g);
    
    // Draw momentum arrows (on top of gravity waves, behind spawn points)
    drawMomentumArrows (g);
}

void Canvas::resized()
{
    // Layout child components here
}

void Canvas::newMassPoint()
{
    if (massPoints.size() < maxMassPoints)
    {
        auto* newMass = new MassPoint();
        massPoints.add (newMass);
        addAndMakeVisible (newMass);
        
        // Set callback to repaint when mass is being moved (every frame during drag)
        newMass->onMassMoved = [this]() { 
            repaint();
        };
        
        // Set callback to repaint when mass is dropped
        newMass->onMassDropped = [this]() { 
            repaint();
            LOG_INFO("Mass dropped - repainting gravity waves");
        };
        
        // Set callback for delete request
        newMass->onDeleteRequested = [this, newMass]() {
            massPoints.removeObject (newMass);
            repaint();
            LOG_INFO("Deleted mass point");
        };
        
        // Place at random position on canvas
        if (getWidth() > 0 && getHeight() > 0)
        {
            juce::Random random;
            int x = random.nextInt (getWidth());
            int y = random.nextInt (getHeight());
            newMass->setCentrePosition (x, y);
            LOG_INFO("Created new mass point at (" + juce::String(x) + ", " + juce::String(y) + ")");
        }
        else
        {
            LOG_WARNING("Canvas bounds not yet set - mass point placed at origin");
        }
        
        // Repaint to show gravity waves for newly added mass
        repaint();
    }
    else
    {
        LOG_WARNING("Maximum mass points reached (" + juce::String(maxMassPoints) + ")");
    }
}

void Canvas::newSpawnPoint()
{
    if (spawnPoints.size() < maxSpawnPoints)
    {
        auto* newPoint = new SpawnPoint();
        spawnPoints.add (newPoint);
        addAndMakeVisible (newPoint);
        
        // Set callback to repaint when spawn point is moved (so arrow moves with it)
        newPoint->onSpawnPointMoved = [this]() { 
            repaint();
        };
        
        // Set callback to repaint when selection changes (so arrow appears/disappears)
        newPoint->onSelectionChanged = [this]() {
            repaint();
        };
        
        // Set callback for delete request
        newPoint->onDeleteRequested = [this, newPoint]() {
            spawnPoints.removeObject (newPoint);
            repaint();
            LOG_INFO("Deleted spawn point");
        };
        
        // Set callback to get spawn point count (for menu state)
        newPoint->getSpawnPointCount = [this]() {
            return spawnPoints.size();
        };
        
        // Place at random position on canvas
        if (getWidth() > 0 && getHeight() > 0)
        {
            juce::Random random;
            int x = random.nextInt (getWidth());
            int y = random.nextInt (getHeight());
            newPoint->setCentrePosition (x, y);
            
            // Set random momentum arrow (random direction and length)
            float randomAngle = random.nextFloat() * juce::MathConstants<float>::twoPi;
            float randomLength = minArrowLength + random.nextFloat() * (maxArrowLength - minArrowLength);
            juce::Point<float> randomMomentum (
                randomLength * std::cos (randomAngle),
                randomLength * std::sin (randomAngle)
            );
            newPoint->setMomentumVector (randomMomentum);
            
            LOG_INFO("Created new spawn point at (" + juce::String(x) + ", " + juce::String(y) + 
                     ") with momentum (" + juce::String(randomMomentum.x) + ", " + 
                     juce::String(randomMomentum.y) + ")");
        }
        else
        {
            LOG_WARNING("Canvas bounds not yet set - spawn point placed at origin");
        }
        
        // Repaint to show arrow for newly added spawn point
        repaint();
    }
    else
    {
        LOG_WARNING("Maximum spawn points reached (" + juce::String(maxSpawnPoints) + ")");
    }
}

void Canvas::drawGravityWaves (juce::Graphics& g)
{
    if (massPoints.isEmpty())
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
    for (auto* spawn : spawnPoints)
    {
        // Only draw arrow for selected spawn points
        if (!spawn->isSelected())
            continue;
        
        juce::Point<float> center = getSpawnPointCenter (spawn);
        juce::Point<float> momentumVector = spawn->getMomentumVector();
        juce::Point<float> arrowEnd = center + momentumVector;
        
        // Draw arrow line with gradient from transparent to opaque
        if (momentumVector.getDistanceFromOrigin() > 2.0f)
        {
            // Draw multiple segments with increasing opacity
            const int numSegments = 20;
            for (int i = 0; i < numSegments; ++i)
            {
                float t1 = (float)i / numSegments;
                float t2 = (float)(i + 1) / numSegments;
                
                juce::Point<float> p1 = center + momentumVector * t1;
                juce::Point<float> p2 = center + momentumVector * t2;
                
                // Fade from 0% to 100% opacity
                float alpha = (t1 + t2) / 2.0f; // Use midpoint opacity for this segment
                g.setColour (juce::Colours::red.withAlpha (alpha));
                g.drawLine (p1.x, p1.y, p2.x, p2.y, 2.0f);
            }
            
            // Draw arrowhead fully opaque
            g.setColour (juce::Colours::red);
            float angle = std::atan2 (momentumVector.y, momentumVector.x);
            float arrowHeadSize = 5.0f;
            
            juce::Point<float> arrowPoint1 (
                arrowEnd.x - arrowHeadSize * std::cos (angle - juce::MathConstants<float>::pi / 6),
                arrowEnd.y - arrowHeadSize * std::sin (angle - juce::MathConstants<float>::pi / 6)
            );
            
            juce::Point<float> arrowPoint2 (
                arrowEnd.x - arrowHeadSize * std::cos (angle + juce::MathConstants<float>::pi / 6),
                arrowEnd.y - arrowHeadSize * std::sin (angle + juce::MathConstants<float>::pi / 6)
            );
            
            g.drawLine (arrowEnd.x, arrowEnd.y, arrowPoint1.x, arrowPoint1.y, 2.0f);
            g.drawLine (arrowEnd.x, arrowEnd.y, arrowPoint2.x, arrowPoint2.y, 2.0f);
        }
    }
}

//==============================================================================
void Canvas::mouseDown (const juce::MouseEvent& event)
{
    juce::Point<float> mousePos = event.position;
    
    // Right-click shows context menu to add mass or spawn point
    if (event.mods.isPopupMenu())
    {
        juce::PopupMenu menu;
        
        // Check limits: max 4 mass points, max 8 spawn points
        bool canAddMass = massPoints.size() < 4;
        bool canAddSpawn = spawnPoints.size() < 8;
        
        // Add menu items (greyed out if at limit)
        menu.addItem (1, "Add Mass Point", canAddMass);
        menu.addItem (2, "Add Spawn Point", canAddSpawn);
        
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (
                           juce::Rectangle<int> (event.getScreenPosition().x, 
                                                event.getScreenPosition().y, 1, 1)),
                           [this, mousePos] (int result)
                           {
                               if (result == 1 && massPoints.size() < 4)
                               {
                                   // Add mass point at mouse position
                                   auto* mass = new MassPoint();
                                   addAndMakeVisible (mass);
                                   mass->setCentrePosition (mousePos.toInt());
                                   mass->onMassDropped = [this]() { repaint(); };
                                   mass->onMassMoved = [this]() { repaint(); };
                                   mass->onDeleteRequested = [this, mass]() {
                                       massPoints.removeObject (mass);
                                       repaint();
                                       LOG_INFO("Deleted mass point");
                                   };
                                   massPoints.add (mass);
                                   LOG_INFO("Added mass point at (" + juce::String(mousePos.x) + ", " + juce::String(mousePos.y) + ") - Total: " + juce::String(massPoints.size()));
                               }
                               else if (result == 2 && spawnPoints.size() < 8)
                               {
                                   // Add spawn point at mouse position
                                   auto* spawn = new SpawnPoint();
                                   addAndMakeVisible (spawn);
                                   spawn->setCentrePosition (mousePos.toInt());
                                   spawn->onSpawnPointMoved = [this]() { repaint(); };
                                   spawn->onSelectionChanged = [this]() { repaint(); };
                                   spawn->onDeleteRequested = [this, spawn]() {
                                       spawnPoints.removeObject (spawn);
                                       repaint();
                                       LOG_INFO("Deleted spawn point");
                                   };
                                   spawn->getSpawnPointCount = [this]() {
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
            draggedArrowSpawnPoint = spawn;
            setMouseCursor (juce::MouseCursor::CrosshairCursor);
            LOG_INFO("Started dragging arrow for spawn point");
            return;
        }
    }
    
    // Check if clicking on a spawn point or mass point
    bool clickedOnComponent = false;
    for (auto* spawn : spawnPoints)
    {
        if (spawn->getBounds().contains (mousePos.toInt()))
        {
            clickedOnComponent = true;
            break;
        }
    }
    if (!clickedOnComponent)
    {
        for (auto* mass : massPoints)
        {
            if (mass->getBounds().contains (mousePos.toInt()))
            {
                clickedOnComponent = true;
                break;
            }
        }
    }
    
    // If clicking on empty canvas (not on any component), deselect all spawn points
    if (!clickedOnComponent)
    {
        for (auto* spawn : spawnPoints)
        {
            spawn->setSelected (false);
        }
        repaint(); // Force repaint to hide arrows immediately
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
        if (length < minArrowLength)
        {
            if (length > 0.0f)
                newVector = (newVector / length) * minArrowLength;
            else
                newVector = juce::Point<float> (minArrowLength, 0.0f); // Default direction if length is 0
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
        auto vector = draggedArrowSpawnPoint->getMomentumVector();
        LOG_INFO("Stopped dragging arrow - momentum: (" + 
                 juce::String(vector.x) + ", " + juce::String(vector.y) + ")");
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
    
    // Lock particles for thread safety
    const juce::ScopedLock lock (particlesLock);
    
    // Check if we need to remove the oldest particle (max 32 particles)
    const int maxParticles = 32;
    if (particles.size() >= maxParticles)
    {
        // Remove the oldest particle (index 0)
        particles.remove (0);
        LOG_INFO("Removed oldest particle to make room (max: " + juce::String(maxParticles) + ")");
    }
    
    // Get the spawn point using round-robin
    auto* spawn = spawnPoints[nextSpawnPointIndex];
    nextSpawnPointIndex = (nextSpawnPointIndex + 1) % spawnPoints.size();
    
    // Get spawn position and initial velocity from momentum arrow
    juce::Point<float> spawnPos = getSpawnPointCenter (spawn);
    juce::Point<float> initialVelocity = spawn->getMomentumVector() * 2.0f; // Scale up for visibility
    
    // Create new particle with canvas bounds and current lifespan setting
    // Default MIDI values: velocity = 1.0 (full), pitch = 1.0 (no shift)
    auto* particle = new Particle (spawnPos, initialVelocity, getLocalBounds().toFloat(), 
                                    particleLifespan, 1.0f, 1.0f);
    particles.add (particle);
    
    LOG_INFO("Spawned particle #" + juce::String(particles.size()) + 
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
    
    // Lock particles for thread safety
    const juce::ScopedLock lock (particlesLock);
    
    // Check if we need to remove the oldest particle (max 32 particles)
    const int maxParticles = 32;
    if (particles.size() >= maxParticles)
    {
        // Remove the oldest particle (index 0)
        particles.remove (0);
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
    
    // Create new particle with MIDI parameters
    auto* particle = new Particle (spawnPos, initialVelocity, getLocalBounds().toFloat(), 
                                    particleLifespan, midiVelocity, pitchShift);
    particles.add (particle);
    
    LOG_INFO("Spawned MIDI particle #" + juce::String(particles.size()) + 
             " - Note: " + juce::String(midiNote) + 
             " (" + juce::MidiMessage::getMidiNoteName(midiNote, true, true, 3) + ")" +
             ", Velocity: " + juce::String(midiVelocity, 2) + 
             ", Pitch: " + juce::String(pitchShift, 3) + "x");
    
    repaint();
}

void Canvas::timerCallback()
{
    const float deltaTime = 1.0f / 60.0f; // 60 FPS
    
    // Update spawn point rotations
    for (auto* spawn : spawnPoints)
    {
        spawn->updateRotation (deltaTime);
        spawn->repaint();
    }
    
    // Update mass point rotations (black holes)
    for (auto* mass : massPoints)
    {
        mass->updateRotation (deltaTime);
        mass->repaint();
    }
    
    // Lock particles for thread safety
    const juce::ScopedLock lock (particlesLock);
    
    // Update all particles
    for (int i = particles.size() - 1; i >= 0; --i)
    {
        auto* particle = particles[i];
        
        // Update canvas bounds for accurate position mapping
        particle->setCanvasBounds (getLocalBounds().toFloat());
        
        // Calculate gravity force from all mass points
        juce::Point<float> totalForce (0.0f, 0.0f);
        
        for (auto* mass : massPoints)
        {
            juce::Point<float> massCenter = juce::Point<float> (
                mass->getBounds().getCentreX(),
                mass->getBounds().getCentreY()
            );
            
            juce::Point<float> particlePos = particle->getPosition();
            juce::Point<float> direction = massCenter - particlePos;
            float distance = direction.getDistanceFromOrigin();
            
            // Avoid division by zero and singularities
            if (distance > 5.0f)
            {
                // Apply mass multiplier based on size
                float effectiveGravity = gravityStrength * mass->getMassMultiplier();
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
        particle->wrapAround (getLocalBounds().toFloat());
        
        // Remove dead particles
        if (particle->isDead())
        {
            LOG_INFO("Particle died after lifetime expired");
            particles.remove (i);
        }
    }
    
    // Repaint to show updated particle positions
    if (!particles.isEmpty())
        repaint();
}

void Canvas::drawParticles (juce::Graphics& g)
{
    const juce::ScopedLock lock (particlesLock);
    
    for (auto* particle : particles)
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
        
        for (auto* particle : particles)
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
                
                // Factor in particle's lifetime (dying particles have less effect)
                float lifetimeAmp = particle->getLifetimeAmplitude();
                influence *= lifetimeAmp;
                
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
