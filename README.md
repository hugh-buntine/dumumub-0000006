# Orbit - Granular Synthesizer Plugin

![Plugin Interface](https://img.shields.io/badge/JUCE-Audio%20Plugin-blue)
![Language](https://img.shields.io/badge/C++-17-red)
![Platform](https://img.shields.io/badge/Platform-macOS%20%7C%20Windows%20%7C%20Linux-green)
![Status](https://img.shields.io/badge/Status-Complete-brightgreen)

A granular synthesizer audio plugin built with JUCE framework, featuring physics-based orbital grain movement, interactive 2D canvas editing, and real-time MIDI-triggered grain emission.

[![Download](https://img.shields.io/badge/Download-dumumub.com-orange?style=for-the-badge)](https://dumumub.com)

## Plugin Interface

![ORBIT Interface](DEMO-MEDIA/GUI.png)

*Granular synthesizer interface with interactive orbital physics canvas*

## Project Overview

Orbit is a granular synthesizer plugin designed for digital audio workstations (DAWs). Grains are spawned via MIDI input and orbit around movable mass points using physics simulation, with their canvas position determining pan and sample playback position in real-time.

**Duration:** August 2025 - February 2026  
**Role:** Solo Developer  
**Technologies:** C++17, JUCE Framework, Granular Synthesis, Physics Simulation, MIDI

## Key Features

### Interactive Orbital Canvas
- **Physics-based Movement** - Grains orbit around mass points using gravitational physics simulation
- **Movable Mass Points** - Drag-and-drop gravity centers with adjustable mass (4 size levels)
- **Multiple Spawn Points** - Up to 8 configurable spawn points with independent momentum arrows
- **Position-based Audio Mapping** - X-axis controls stereo pan, Y-axis controls sample playback position

![Canvas Interaction](DEMO-MEDIA/CANVAS.gif)

*Interactive canvas with draggable mass points and spawn points*

![Orbit Physics](DEMO-MEDIA/ORBIT.gif)

*Grains orbiting around mass points with real-time physics simulation*

### MIDI-Triggered Grain Emission
- **Velocity-based Speed** - MIDI velocity controls initial grain orbital velocity
- **Pitch Mapping** - MIDI note number determines grain pitch shift
- **Polyphonic Spawning** - Multiple grains can be active simultaneously (configurable limit)
- **Momentum Arrows** - Draggable arrows on spawn points set initial grain trajectory

![MIDI Spawning](DEMO-MEDIA/MIDI.gif)

*MIDI-triggered grain spawning with velocity-based orbital speed*

![Momentum Arrows](DEMO-MEDIA/ARROWS.gif)

*Adjustable momentum arrows controlling initial grain direction*

### Granular Synthesis Engine
- **Grain Size Control** - Adjustable grain duration (10-500ms)
- **Grain Frequency** - Control grain triggering rate for density
- **ADSR Envelope** - Full Attack, Decay, Sustain, Release control per grain
- **Multi-format Support** - Load WAV, MP3, AIFF, FLAC audio files via drag-and-drop

![ADSR Control](DEMO-MEDIA/ADSR.gif)

*Real-time ADSR envelope adjustment with visual feedback*

![Audio Loading](DEMO-MEDIA/AUDIODROP.gif)

*Drag-and-drop audio file loading with waveform display*

### Canvas Elements
- **Mass Points (Vortex)** - Animated rotating vortex graphics with 4 size states
- **Spawn Points (Spawner)** - Dual-layer rotating spawner graphics with momentum arrows
- **Particle Trails** - Visual trail system showing grain orbital paths
- **Edge Behavior** - Toggle between bounce mode and screen wrapping

![Mass Point Sizes](DEMO-MEDIA/MASSSIZE.gif)

*Adjustable mass point sizes affecting gravitational pull strength*

![Bounce Mode](DEMO-MEDIA/BOUNCE.gif)

*Toggle between wrap-around and bounce edge behavior*

### Professional UI/UX
- **Custom Graphics** - Hand-designed interface with hover states for all controls
- **Visual Parameter Feedback** - ADSR curve visualization, grain size waveform preview
- **Gain Visualization** - Scaling knob with -infinity dB support
- **Responsive Controls** - Custom slider look-and-feel with rotation animation

![Parameter Feedback](DEMO-MEDIA/SLIDERS.gif)

*Interactive sliders with visual parameter feedback*
