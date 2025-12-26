# Logging Guide for dumumub-0000006

## Overview
The plugin includes a comprehensive logging system to help debug audio glitches and clicks. Logs are written to:
```
/Users/hughbuntine/Desktop/DUMUMUB/DUMUMUB PLUGINS/dumumub-0000006/logs/dumumub-0000006.log
```

## Enabling/Disabling Logging

### Toggle Logging Parameter
A boolean parameter `enableLogging` has been added to control logging:
- **Default**: Enabled (true)
- **Location**: In the plugin's parameter tree (APVTS)
- **Access**: Can be automated in your DAW or controlled programmatically

### How to Toggle
The logging state is controlled by the `enableLogging` parameter. When changed, it will automatically enable or disable all logging throughout the plugin.

## What Gets Logged

### Critical Errors Only (Audio Thread Safe)
The logging has been carefully designed to avoid performance issues. Only critical errors that could cause clicks are logged:

#### 1. **Buffer Boundary Violations**
- **When**: When grain playback tries to read outside the audio buffer
- **Why it matters**: Can cause silence or pops
- **Frequency**: Logged every 1000th occurrence to avoid spam
- **Example**: `BUFFER BOUNDS VIOLATION: sourceSample=45123, bufferLength=44100, grainStart=1000, pitchShift=1.523 (count: 3000)`

#### 2. **Invalid Grain Amplitudes**
- **When**: NaN or Infinity values detected in grain envelope calculations
- **Why it matters**: Will cause loud digital noise/clicks
- **Frequency**: Logged every 500th occurrence
- **Example**: `INVALID GRAIN AMPLITUDE: NaN or Inf detected, playbackPos=2341 (count: 500)`

#### 3. **High Grain Count Warning**
- **When**: More than 50 grains are active simultaneously
- **Why it matters**: Can cause CPU overload and audio dropouts/clicks
- **Frequency**: Only when count jumps by 20+ grains
- **Example**: `HIGH GRAIN COUNT: 72 active grains (may cause CPU issues/clicks)`

#### 4. **Very Short Grain Warning**
- **When**: Grain size is too short for proper Hann windowing
- **Why it matters**: Falls back to linear fade which may click
- **Frequency**: Logged once per session
- **Example**: `Using LINEAR FADE for very short grain (8 samples, 0.18 ms @ 44.1kHz) - may cause clicks`

#### 5. **Invalid Envelope Values**
- **When**: Envelope calculation produces NaN/Inf
- **Why it matters**: Indicates math error that will cause clicks
- **Frequency**: Every 500th occurrence
- **Example**: `INVALID ENVELOPE VALUE (NaN/Inf): normalizedPos = 1.2343 (occurrence #500)`

#### 6. **Grain Size Jumps**
- **When**: Grain size parameter changes by more than 10ms suddenly
- **Why it matters**: Sudden grain size changes create audible discontinuities
- **Frequency**: Each time it occurs
- **Example**: `GRAIN SIZE JUMP: 50.0ms -> 15.0ms (sudden changes can click)`

#### 7. **Extreme Pitch Shifts**
- **When**: Pitch shift is <0.25x or >4.0x
- **Why it matters**: Can cause aliasing artifacts and clicks
- **Frequency**: Every 100th occurrence
- **Example**: `EXTREME PITCH SHIFT: 5.234x (may cause aliasing, count: 100)`

#### 8. **Voice Stealing Events**
- **When**: MAX_GRAINS_PER_PARTICLE reached and oldest grain is removed
- **Why it matters**: Abrupt grain removal causes clicks
- **Frequency**: Every 50th occurrence
- **Example**: `VOICE STEALING: Max grains reached (32), removed grain at pos 1234 (count: 50)`

#### 9. **Grain Start Position Jumps**
- **When**: Grain start position jumps >5% of buffer length
- **Why it matters**: Rapid Y-position changes cause phase discontinuities
- **Frequency**: Every 100th occurrence (when jumps are frequent)
- **Example**: `GRAIN START POSITION JUMP: 10000 -> 50000 (delta: 40000 samples, may cause phase issues)`

### UI Events (Always Logged When Enabled)
Non-audio-thread events are logged normally:
- MIDI note on/off events
- Button clicks (Graphics, Break CPU)
- Particle/mass/spawn point creation/deletion
- Mouse interactions (drag, hover, menu selections)
- Audio file loading
- Canvas resize events

## Performance Considerations

### Audio Thread Safety
- **No logging in inner audio loops**: Prevents dropouts
- **Error detection only**: Only anomalies trigger logs
- **Rate limiting**: Most errors are throttled to avoid log spam
- **Minimal CPU impact**: Logging checks are lightweight

### When to Enable
- **During development**: Keep enabled to catch issues
- **For debugging clicks**: Essential for diagnosing grain synthesis problems
- **In production**: Can leave enabled - very low overhead
- **For performance testing**: Disable to eliminate any logging overhead

## Interpreting the Logs

### Click Diagnosis Workflow

1. **Enable logging** (if not already enabled)
2. **Reproduce the click** 
3. **Check the log file** for errors around the time of the click
4. **Look for patterns**:
   - Multiple boundary violations → Audio buffer too small or pitch shift too extreme
   - Invalid amplitudes → Math error in ADSR or grain envelope
   - High grain count → Too many particles or grain frequency too high
   - Short grain warnings → Grain size parameter set too low

### Common Issues

**"BUFFER BOUNDS VIOLATION" frequently**
- Audio file may be corrupt or incorrectly loaded
- Pitch shift values are extreme
- Grain start positions are calculated incorrectly

**"INVALID GRAIN AMPLITUDE"**
- ADSR envelope has math error
- Grain playback position is corrupted

**"GRAIN SIZE JUMP"**
- User is rapidly changing grain size parameter
- Automation is creating sudden jumps
- **Fix**: Use smoother parameter automation

**"EXTREME PITCH SHIFT"**
- Particle physics is creating very fast movements
- Mass points or canvas settings causing high velocities
- **Fix**: Adjust mass point gravity or particle spawn velocity

**"VOICE STEALING"**
- Too many grains overlapping (grain frequency too high)
- Grain size too long causing buildup
- **Fix**: Lower grain frequency or reduce grain size

**"GRAIN START POSITION JUMP"**
- Particle Y position changing rapidly (likely from physics)
- Mouse dragging particles quickly
- **Fix**: Check mass point attraction strength or damping
- Division by zero in envelope calculation

**"HIGH GRAIN COUNT: >100 grains"**
- Too many particles spawned
- Grain frequency set too high
- Particles not being cleaned up properly

**"Using LINEAR FADE for very short grain"**
- Grain Size parameter set below ~0.5ms
- Will likely cause clicks - increase grain size

## Log File Format

Each log entry includes:
- Timestamp
- Log level: `[INFO]`, `[WARNING]`, `[ERROR]`
- Message with context

Example:
```
2025-12-26 15:30:45.123 [ERROR] BUFFER BOUNDS VIOLATION: sourceSample=45123, bufferLength=44100, grainStart=1000, pitchShift=1.523 (count: 1000)
2025-12-26 15:30:45.234 [WARNING] HIGH GRAIN COUNT: 67 active grains (may cause CPU issues/clicks)
2025-12-26 15:30:46.456 [INFO] MIDI Note On received: note=60 velocity=0.75
```

## Disabling Specific Log Types

If you want to modify what gets logged, edit these files:
- `source/PluginProcessor.cpp` - Audio processing logs
- `source/Particle.cpp` - Grain envelope calculation logs
- `source/Canvas.cpp` - UI interaction logs
- `source/MassPoint.cpp` - Mass point interaction logs
- `source/SpawnPoint.cpp` - Spawn point interaction logs

Look for `LOG_ERROR()`, `LOG_WARNING()`, and `LOG_INFO()` calls.

## Tips

1. **Check logs after each session** to catch issues early
2. **Archive logs** before major changes to compare before/after
3. **Use log patterns** to identify systematic issues vs. one-off glitches
4. **Correlate with audio** - note the timestamp when you hear a click
5. **Share logs** when reporting bugs - include the full log file
