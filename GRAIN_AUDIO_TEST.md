# Grain Audio Playback Test Guide

## What's Been Implemented

✅ **Continuous Granular Synthesis Engine**
- Each particle continuously plays grains from the loaded audio file (50ms grains with no gap)
- X-axis position controls stereo panning (left to right) **in real-time**
- Y-axis position determines which part of the audio sample plays **dynamically as particles move**
- Grain envelope with attack/release (5ms each)
- Grains automatically restart based on current position

✅ **Audio Features**
- **Real-time position tracking** - grain sample position updates based on particle's current Y coordinate
- Equal-power stereo panning based on particle X position
- Continuous grain emission - particles never stop making sound (until they die after 30s)
- Sample position mapping: top of canvas = start of audio, bottom = end of audio
- Automatic grain envelope for smooth playback
- Thread-safe particle access between UI and audio threads
- Visual feedback: particles stay **RED while alive and playing**

## How to Test

1. **Launch the plugin** (should have opened automatically after build)

2. **Load an audio file:**
   - Drag and drop a WAV, MP3, AIFF, or FLAC file onto the canvas
   - You should see the waveform drawn in the center of the canvas
   - The filename will appear at the top

3. **Create particles:**
   - Click the "Emit Particle" button at the bottom
   - Watch the particles orbit around the mass point
   - **Particles stay RED** and continuously play audio as they orbit
   - Each particle plays non-stop 50ms grains based on its current position

4. **Test positional audio (this is the cool part!):**
   - **LEFT/RIGHT panning:** As particles orbit, sound pans smoothly left to right
   - **Dynamic sample position:** As particles move up/down, you hear different parts of the audio
   - Top of canvas = plays from START of audio sample
   - Bottom of canvas = plays from END of audio sample
   - **Orbiting creates evolving textures** - as Y position changes during orbit, the timbre morphs!

5. **Experiment with spawn positions:**
   - Drag the spawn point (✛) to different Y positions
   - Higher spawn = particles play from earlier in the sample
   - Lower spawn = particles play from later in the sample

6. **Test continuous emission:**
   - Click "Emit Particle" multiple times rapidly
   - You should hear overlapping grains creating dense texture
   - **Each particle is its own continuous voice** playing grains

7. **Listen to the orbital effect:**
   - Single particle creates a **morphing loop** as it orbits
   - The timbre changes as Y position varies during orbit
   - Panning sweeps as X position changes
   - This creates interesting **evolving textures automatically**

## Parameters Currently Active

- **Grain Size:** 50ms (hardcoded)
- **Attack:** 5ms
- **Release:** 5ms
- **Grain Lifespan:** 30 seconds
- **Gravity Strength:** 50,000 (affects orbit speed)

## Expected Behavior

✅ **When a particle spawns:**
- It immediately starts playing audio (RED color)
- It **continuously plays** 50ms grains with no gaps
- Sample position updates in real-time based on Y coordinate
- Panning updates in real-time based on X coordinate
- It continues until it dies after 30 seconds

✅ **Panning is smooth and continuous:**
- As particles orbit left to right, sound pans accordingly in real-time
- Multiple particles create rich spatial texture

✅ **Y-position creates evolving timbre:**
- As particles orbit up/down, you hear different parts of the audio
- Creates **automatic morphing textures** from orbital motion
- Circular orbits = repeating but evolving patterns

✅ **The magic of orbital motion:**
- Each particle is like a **moving playhead** through your audio
- Gravity creates natural rhythmic patterns
- Different orbit speeds = different grain repetition rates

## Known Limitations (To Be Implemented)

- ❌ No parameter controls yet (grain size, etc. are hardcoded)
- ❌ No MIDI input (using button to simulate)
- ❌ No polyphony limiting (unlimited grains can overlap)
- ❌ No master gain control
- ❌ Grain size is fixed at 50ms

## Next Steps

1. Add UI sliders for grain parameters
2. Implement MIDI note triggering
3. Add polyphony limiting
4. Add adjustable ADSR
5. Add velocity sensitivity

## Troubleshooting

**No sound?**
- Make sure an audio file is loaded (drag and drop onto canvas)
- Check your audio output device is enabled
- Try clicking "Emit Particle" - you should hear continuous sound

**Particles turn BLUE?**
- This shouldn't happen anymore! Particles should stay RED
- If they turn blue, they've died (after 30 seconds of life)

**Sound is too dense/overwhelming?**
- Try fewer particles (they play continuously now)
- This is expected - will add volume/polyphony controls later

**Crackling/pops?**
- This is expected with rapid grain emissions
- Will be smoothed with better envelope control later
