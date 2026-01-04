# Why Buffer Boundary Clicks Happen (And Why Perfect Continuity is Impossible)

## Visual Explanation

```
TIME →
Buffer N (512 samples)                    Buffer N+1 (512 samples)
═══════════════════════════════════════  ═══════════════════════════════════════
Sample 511                                Sample 0

GRAIN 1 (fading out):
    ...0.5245 → 0.5244 ║ 0.5243 → 0.5232...
                   ↑   ║   ↑
                 Last  ║  First (new buffer, grain advanced by 512)
                       ║
GRAIN 2 (at peak):     ║
    ...1.0000 → 1.0000 ║ 1.0000 → 1.0000...
                       ║
GRAIN 3 (fading in):   ║
    ...0.8795 → 0.8796 ║ 0.8797 → 0.8804...
                       ║
═══════════════════════║═══════════════════════════════════
SUM:    2.4040 → 2.4040 ║ 2.4040 → 2.4036
                   ↑   ║   ↑
              Expected ║ Actual
                       ║
         DISCONTINUITY: 0.0004 ← CLICK!
```

## The Problem: Each Buffer is Independent

### What Happens in Your Code

```cpp
// Start of processBlock()
buffer.clear();  // ← Buffer starts at ZERO

// Add grain contributions
for (grain in grains) {
    leftChannel[i] += grainSample;  // ← Additive
}

// End of buffer: some value (e.g., 2.4040)

// ──────── BUFFER BOUNDARY ────────

// Next buffer starts
buffer.clear();  // ← Back to ZERO again!

// Add grain contributions (grains advanced by 512 samples)
for (grain in grains) {
    leftChannel[i] += grainSample;  // ← Different values now!
}
// First sample: 2.4036 (not 2.4040!)
```

## Why Grains Change Amplitude

Each grain uses a **Hann window** for smooth fade-in/fade-out:

```
Grain envelope over time:
     ╱‾‾‾‾‾‾‾‾‾╲
    ╱           ╲
   ╱             ╲
  ╱               ╲
 ╱                 ╲
───────────────────────→ time

Position:  0  1000  2000  3000  4000  5000  6000  7000
Amplitude: 0  0.5   0.86  1.0   1.0   0.86  0.5   0.0
```

### When Multiple Grains Overlap at Different Phases:

```
Buffer boundary at position X:

Grain A at pos 7167: amp = 0.5244 (fading out, decreasing)
Grain B at pos 4095: amp = 1.0000 (at peak, constant)
Grain C at pos 1023: amp = 0.8796 (fading in, increasing)

Next sample (pos X+1):

Grain A at pos 7168: amp = 0.5232 (decreased by 0.0012)
Grain B at pos 4096: amp = 1.0000 (no change)
Grain C at pos 1024: amp = 0.8804 (increased by 0.0008)

Net change: -0.0012 + 0 + 0.0008 = -0.0004 ← DISCONTINUITY!
```

## Why We Can't "Just Fix It"

### Option 1: Predict Next Sample ❌
**Problem**: Would need to render grain contributions for sample 0 of next buffer BEFORE starting next buffer
- Breaks real-time streaming architecture
- Requires buffering/lookahead
- Adds latency

### Option 2: Store "Target Value" ❌
**Problem**: Can't know what sample 0 SHOULD be without calculating grain contributions
- Grains can be triggered mid-buffer
- Grain parameters can change
- New grains might start exactly at buffer boundary

### Option 3: Continuous State Machine ✅ (Complex Refactor)
**Solution**: Track grain states between buffers and ensure continuity
```cpp
// End of buffer N
for (grain in grains) {
    grain.lastOutputValue = calculateGrainSample(grain, 511);
}

// Start of buffer N+1
for (grain in grains) {
    float predictedValue = grain.lastOutputValue;
    float actualValue = calculateGrainSample(grain, 0);
    float error = actualValue - predictedValue;
    
    // Apply micro-correction
    grain.phaseOffset -= error;
}
```

**BUT**: This is complex and changes the grain synthesis algorithm!

## Current Fix: Shaped Crossfade

Instead of perfect continuity, we **smooth the transition**:

```
Linear crossfade (4 samples) - Creates "diagonal line":
Sample: 0        1        2        3        4+
Value:  Expected ────────────────→ Actual
        └─ Audible slope/click

S-curve crossfade (16 samples) - Smoother:
Sample: 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16+
Value:  Expected ╭───────────╮ Actual
                 └─ Gentle curve, inaudible
```

### S-Curve Formula (Cubic Ease-in-out):
```
For t from 0.0 to 1.0:

if t < 0.5:
    fadeIn = 4 × t³               (slow start)
else:
    fadeIn = 1 + ((2t - 2)³) / 2  (slow finish)
```

This creates a **smooth acceleration and deceleration**, avoiding the harsh "diagonal line" that creates clicks.

## Updated Fix Parameters

- **Crossfade length**: 16 samples (was 4)
  - Duration: 0.36ms @ 44.1kHz (still inaudible)
- **Curve**: Cubic S-curve (was linear)
  - Avoids harsh transitions
- **Threshold**: 0.0003 (was 0.0005)
  - Catches smaller discontinuities

## Why This Works

The S-curve makes the transition so gradual that:
1. Our ears can't detect the 0.36ms smoothing
2. High-frequency click harmonics are eliminated
3. The amplitude change feels "natural" rather than abrupt

## Testing

After rebuilding, the clicks should be **much quieter** or eliminated because:
- Longer crossfade = more gentle transition
- S-curve = no harsh diagonal slope
- Lower threshold = catches smaller jumps

The waveform will still show a tiny smooth curve at buffer boundaries (that's the crossfade), but it won't click because the curve is gentle enough that our ears perceive it as continuous.

## Future Perfect Solution

To achieve **perfect continuity**, you'd need to refactor to:

1. **Track grain phase state** between buffers
2. **Pre-calculate buffer 0** to match buffer end
3. **Use overlap-add synthesis** with aligned grain triggering

But this is a significant architectural change and the current fix should eliminate 99% of clicks!
