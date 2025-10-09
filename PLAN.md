# DUMUMUB-0000006: Orbital Grains

## Core Concept
MIDI-triggered grains spawn on 2D canvas and orbit around movable center mass. Position controls pan + sample position. Pitch is static (MIDI note). Volume controlled by lifespan.

---

## Parameters (Sliders/Knobs)

- **Grain Size** (5-500ms) - Duration of each grain
- **Grain Lifespan** (0.1-infinite) - How long before fade out
- **ADSR** - Attack, Decay, Sustain, Release envelope
- **Gravity Strength** - Pull force toward center mass
- **Velocity Scale** (0-200%) - Multiplier for MIDI velocity → grain speed
- **Max Grains** - Polyphony limit (8, 16, 32, 64, unlimited)
- **Master Gain** - Output volume

---

## 2D Canvas Stuff

### Movable Elements
- **Center Mass** (Click drag) - Gravity point
- **Spawn Point** (Click drag) - Where MIDI notes spawn
- **Momentum Arrow per Spawn Point** - Draggable arrow to set initial velocity direction/strength of each spawn point

### Position Mappings
- **X-axis = Pan** (left/right stereo)
- **Y-axis = Sample Position** (which part of audio buffer)

### Visual Elements
- Grain particles (colored dots, fade with age)
- Optional trails showing grain paths
- Optional velocity vectors
- Gravity field rings around center mass

---

## MIDI Behavior

- **Note On** → Spawn grain at spawn point, Y-position based on MIDI note
- **Note Off** → Grain continues until lifespan ends (or option to fade immediately)
- **MIDI Velocity** → Controls initial grain speed (how fast it orbits)
  - Low velocity = slow orbit
  - High velocity = fast orbit
  - Scaled by "Velocity Scale" parameter
- **Pitch** → Static, doesn't change while orbiting

---

## Physics

- **No air resistance** - Perfect circular orbits
- Grains orbit until lifespan kills them
- User can move center mass in real-time
- Tangential velocity creates stable orbits
- Particles can bounce off canvas edges or wrap around (modifiable)
- Multiple spawn points with independent momentum arrows

---

## File Loading

- Load audio sample (WAV, MP3, AIFF, FLAC)
- Drag-and-drop support
- Display loaded file name

---

## Remember

- Pitch = MIDI note (static, no modulation)
- Pan changes as grain moves left/right (stereo orbit effect)
- Sample position changes as grain moves up/down (timbre morphs)
- Volume only controlled by lifespan ADSR
- **Initial speed = MIDI velocity** (harder hit = faster orbit)
- Velocity Scale parameter lets user adjust sensitivity
- No drag/friction = stable orbits
- Each spawn point can have its own momentum arrow (sets direction)
- Momentum arrow + MIDI velocity = final initial velocity
- User plays their own melodies

---

<div style="page-break-after: always;"></div>

## UI Mockup

```
NOT ACTUAL UI
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
║  ORBITAL GRAINS                                      [Load Sample ▼]     ║
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
║                                                                          ║
║   ┌────────────────────────────────────────────────────────────────┐     ║
║   │                         GRAIN FIELD                            │     ║
║   │                                                                │     ║
║   │          ●                      ○                              │     ║
║   │                                                                │     ║
║   │     ●          ✛ SPAWN                                         │     ║
║   │                 ↗ momentum                                     │     ║
║   │                                                                │     ║
║   │                        ╳ MASS                ●                 │     ║
║   │              ●         ◯◯◯                                     │     ║
║   │                                    ●                           │     ║
║   │                                                                │     ║
║   │     ●                                      ○                   │     ║
║   │                                                                │     ║
║   │                            ●                                   │     ║
║   └────────────────────────────────────────────────────────────────┘     ║
║   Left Pan ← X-axis → Right Pan  |  Bottom Sample ← Y-axis → Top Sample  ║
║                                                                          ║
║   Grains: 12/64    Sample: drum_loop.wav                                 ║
║                                                                          ║
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
║                                                                          ║
║   GRAIN CONTROLS              PHYSICS                  ADSR              ║
║   ┌─────────────────────┐    ┌──────────────────┐    ┌─────────────┐     ║
║   │ Grain Size          │    │ Gravity Strength │    │ Attack      │     ║
║   │ [████▌──────] 50ms  │    │ [███████▌───] 75 │    │ [██▌────] 5 │     ║
║   │                     │    │                  │    │ Decay       │     ║
║   │ Grain Lifespan      │    │ Velocity Scale   │    │ [████▌──] 8 │     ║
║   │ [██████──] 2.5s     │    │ [█████▌─────] 50 │    │ Sustain     │     ║
║   │                     │    │                  │    │ [██████] 10 │     ║
║   │ Max Grains          │    │ Launch Angle     │    │ Release     │     ║
║   │ [████────] 64       │    │ [═══●═══] 45°    │    │ [███▌───] 6 │     ║
║   │                     │    │                  │    │             │     ║
║   │ Master Gain         │    │ [Toggle Trails]  │    │             │     ║
║   │ [█████▌─────] -6dB  │    │ [Show Vectors]   │    │             │     ║
║   └─────────────────────┘    └──────────────────┘    └─────────────┘     ║
║                                                                          ║
║   💡 Drag center mass (╳) to move gravity point                          ║
║   💡 Drag spawn point (✛) to set where notes appear                      ║
║   💡 Drag momentum arrow (↗) to set initial orbit direction              ║
║                                                                          ║
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛

Legend:
  ● = Active grain (moving)
  ○ = Fading grain (dying)
  ╳ = Center mass (gravity point)
  ✛ = Spawn point (where MIDI notes appear)
  ↗ = Momentum arrow (sets initial velocity)
  ◯◯◯ = Gravity field rings
```
