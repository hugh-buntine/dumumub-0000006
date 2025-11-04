# Grain Timing & Envelope Explanation

## 📊 Current Grain Timing

### Grain Duration: **50ms**
Each grain plays for exactly 50 milliseconds of audio.

### Envelope Structure (within each 50ms grain):
```
Total: 50ms
├── Attack:  5ms  (fade in from 0 to 1.0)
├── Sustain: 40ms (hold at 1.0)
└── Release: 5ms  (fade out from 1.0 to 0)
```

### Gap Between Grains: **ZERO**
- As soon as one grain finishes (after 50ms), the next grain starts immediately
- This creates **continuous, uninterrupted audio playback**
- Think of it like a seamless loop

## 🎵 How the Envelope Prevents Clicks

### Without Envelope (would cause clicks):
```
Sample 1:  |████████████████████████| (full volume, abrupt start)
Sample 2:  |████████████████████████| (full volume, abrupt end)
           ↑ CLICK!                ↑ CLICK!
```

### With Envelope (smooth, no clicks):
```
Sample 1:  |/▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔\| (fade in/out)
Sample 2:  |/▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔\| (fade in/out)
           ↑ smooth              ↑ smooth
```

The 5ms attack fades FROM zero, and the 5ms release fades TO zero, preventing abrupt transitions.

## 🔄 Continuous Grain Flow Timeline

```
Time:     0ms    50ms   100ms   150ms   200ms
Grain 1:  [====] 
Grain 2:         [====]
Grain 3:                [====]
Grain 4:                       [====]
Grain 5:                              [====]
          ^ no gap    ^ no gap  ^ no gap

Result: Continuous audio stream with smooth envelopes
```

## 📍 Dynamic Sample Position

The clever part: **each grain starts at a different sample position** based on the particle's current Y position:

```
Particle at Y=100:  plays samples 1000-3205 (50ms)
  ↓ particle moves to Y=150
Particle at Y=150:  plays samples 5000-7205 (50ms)
  ↓ particle moves to Y=200
Particle at Y=200:  plays samples 8500-10705 (50ms)
```

This creates **evolving textures** as particles orbit!

## 🎛️ Current Parameters

| Parameter | Value | Notes |
|-----------|-------|-------|
| **Grain Size** | 50ms | Total duration of each grain |
| **Attack Time** | 5ms | Fade in duration |
| **Release Time** | 5ms | Fade out duration |
| **Sustain Time** | 40ms | Full volume duration (50 - 5 - 5) |
| **Gap Between Grains** | 0ms | Seamless, continuous playback |
| **Sample Rate** | Auto-detected | Uses your audio interface's sample rate |

## 🔧 Sample Rate Handling

The system now properly detects your audio interface's sample rate:
- **44.1kHz:** 50ms = 2,205 samples
- **48kHz:** 50ms = 2,400 samples
- **96kHz:** 50ms = 4,800 samples

Attack/release envelopes scale accordingly!

## 🎨 Visual Feedback

- **RED particles** = actively playing grains (continuous)
- **BLUE particles** = dead (after 30 seconds of life)
- Particles should stay RED as long as they're alive and audio is loaded

## 💡 Why This Works

1. **5ms attack** prevents clicks at grain start (smooth fade in)
2. **5ms release** prevents clicks at grain end (smooth fade out)
3. **Zero gap** ensures continuous sound (no silence between grains)
4. **Dynamic position** creates evolving textures (each grain plays different audio)
5. **Proper sample rate** ensures accurate timing on all audio interfaces

## 🎯 Next Steps for Control

Future parameters you could add:
- **Grain Size slider:** 10ms - 500ms (currently fixed at 50ms)
- **Attack/Release sliders:** 1ms - 50ms (currently fixed at 5ms)
- **Grain Density:** Add gaps between grains for sparser textures
- **Randomization:** Vary grain parameters slightly for organic feel

## 🔊 Testing Tips

1. **Use percussive audio** (drums, clicks) to hear the positional changes clearly
2. **Use sustained audio** (synth pad, drone) to hear the smooth envelope
3. **Watch Y-position changes** as particles orbit - you should hear timbre morph
4. **Multiple particles** create dense, layered textures
5. **Different spawn heights** = particles play different parts of the sample

The combination of continuous grains + smooth envelopes + dynamic positioning creates the signature "Orbital Grains" sound!
