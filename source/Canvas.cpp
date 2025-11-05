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
    
    // Draw ripples emanating from each mass point
    g.setColour (juce::Colours::grey.withAlpha (0.3f));
    
    int numRipples = 10;
    float maxRadius = 200.0f;
    
    for (auto* mass : massPoints)
    {
        juce::Point<float> center (mass->getBounds().getCentreX(), 
                                   mass->getBounds().getCentreY());
        
        // Scale ripple intensity and spacing based on mass multiplier
        float intensityScale = mass->getMassMultiplier();
        
        // Larger masses have tighter/more frequent ripples
        int adjustedNumRipples = static_cast<int>(numRipples * intensityScale);
        
        // Draw concentric circles (more ripples = tighter spacing)
        for (int i = 1; i <= adjustedNumRipples; ++i)
        {
            float radius = (maxRadius / adjustedNumRipples) * i;
            float alpha = 0.3f * (1.0f - (float)i / adjustedNumRipples) * juce::jmin(intensityScale, 2.0f);
            
            g.setColour (juce::Colours::grey.withAlpha (alpha));
            g.drawEllipse (center.x - radius, center.y - radius, 
                          radius * 2, radius * 2, 1.0f);
        }
    }
    
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
    g.setColour (juce::Colours::red);
    
    for (auto* spawn : spawnPoints)
    {
        juce::Point<float> center = getSpawnPointCenter (spawn);
        juce::Point<float> momentumVector = spawn->getMomentumVector();
        juce::Point<float> arrowEnd = center + momentumVector;
        
        // Draw arrow line
        g.drawLine (center.x, center.y, arrowEnd.x, arrowEnd.y, 2.0f);
        
        // Draw arrowhead
        if (momentumVector.getDistanceFromOrigin() > 2.0f)
        {
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
    
    // Check if clicking near any arrow tip
    for (auto* spawn : spawnPoints)
    {
        if (isMouseNearArrowTip (spawn, mousePos))
        {
            draggedArrowSpawnPoint = spawn;
            setMouseCursor (juce::MouseCursor::CrosshairCursor);
            LOG_INFO("Started dragging arrow for spawn point");
            return;
        }
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
    
    // Draw horizontal line every 2 vertical pixels
    g.setColour (juce::Colours::black);
    
    for (int y = 0; y < canvasHeight; y += 2)
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
        
        // Draw horizontal line centered in canvas
        if (lineHalfWidth > 0.5f)
        {
            g.drawLine (centerX - lineHalfWidth, static_cast<float>(y),
                       centerX + lineHalfWidth, static_cast<float>(y),
                       1.0f);
        }
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
