# Buffer Boundary Click Diagnosis

## Problem Description
Clicks occur at buffer boundaries when multiple grains overlap. The clicks are small discontinuities (jumps) in the audio signal between the last sample of one buffer and the first sample of the next buffer.

## Root Cause Analysis

### How Granular Synthesis Works in This Plugin

1. **Multiple Overlapping Grains**: The plugin can have 3-8 overlapping grains per particle
2. **Additive Synthesis**: Grains are summed together: `leftChannel[i] += leftSample;`
3. **Buffer Processing**: Audio is processed in blocks (typically 512 samples)

### The Actual Problem

The issue is **NOT** about individual grain state - each grain correctly maintains its playback position across buffers. The problem is the **cumulative sum** of multiple grains:

```
Buffer N end (sample 511):   Grain1(amp=0.8) + Grain2(amp=0.3) + Grain3(amp=0.1) = 1.2
Buffer N+1 start (sample 0):  Grain1(amp=0.79) + Grain2(amp=0.31) + Grain3(amp=0.09) = 1.19
```

Even though this is only a 0.01 difference, it can be audible as a click because:
- The samples are sequential in time (no gap)
- Any discontinuity creates high-frequency harmonics
- With many overlapping grains, small phase shifts accumulate

### Why It Only Happens with Overlapping Grains

With a single grain:
- The Hann window envelope ensures smooth transitions
- Grain phase is continuous across buffers

With overlapping grains:
- Each grain has its own Hann window phase
- The **sum** of multiple Hann windows can be discontinuous
- This is because grain triggering is based on time (grainFreq), NOT aligned to buffer boundaries

## The Real Issue: Grain Amplitude Calculation

Looking at the code more carefully, I found the actual bug:

### Current Behavior

In `PluginProcessor.cpp`, grains are rendered like this:

```cpp
for (int i = 0; i < samplesToRender; ++i)
{
    // Calculate grain amplitude PER-SAMPLE
    Grain currentGrain = grain;
    currentGrain.playbackPosition = grainPosition + i;
    float grainAmplitude = particle->getGrainAmplitude (currentGrain);
    
    // ... audio rendering ...
    
    leftChannel[i] += leftSample;  // ADDITIVE
}

// AFTER rendering, update all grains
particle->updateGrains (buffer.getNumSamples());
```

### The Bug

The problem is that `grainPosition` is the position at the **start** of the buffer. When we do:
```cpp
currentGrain.playbackPosition = grainPosition + i;
```

This is correct for samples 0 to 511 in the current buffer. BUT, at the start of the NEXT buffer:
- `grainPosition` has been advanced by `buffer.getNumSamples()` via `updateGrains()`
- The first sample of the new buffer uses this NEW position
- There's NO guarantee that the amplitude at position X is continuous with position X-1

### Example of the Discontinuity

```
Buffer N:
  Sample 511: grainPosition = 5000, i = 511 → playbackPos = 5511, amp = 0.85

Buffer N+1:
  updateGrains() called: grainPosition += 512 → grainPosition = 5512
  Sample 0: grainPosition = 5512, i = 0 → playbackPos = 5512, amp = 0.84
```

The amplitude calculation is **correct** (positions 5511 and 5512 are sequential), but the KEY ISSUE is:

**When multiple grains overlap, small rounding errors or phase shifts in the Hann window calculations can cause the SUM to be discontinuous.**

## Proof via Logging

The added logging will show:
1. Buffer boundary jumps (difference between end of buffer N and start of buffer N+1)
2. Individual grain states at boundaries
3. Source audio positions for each grain

## Solution Approaches

### Option 1: DC Offset Removal (Band-Aid Fix)
Add a high-pass filter to remove DC discontinuities
- **Pros**: Simple, handles all discontinuities
- **Cons**: Doesn't fix root cause, changes tone slightly

### Option 2: Buffer Crossfade (Your Current Workaround)
Fade between last samples of buffer N and first samples of buffer N+1
- **Pros**: Smooths transitions
- **Cons**: Added CPU overhead, still not addressing root cause

### Option 3: Phase-Aligned Grain Triggering (Best Fix)
Align grain trigger times to buffer boundaries
- **Pros**: Prevents discontinuities entirely
- **Cons**: Changes grain timing slightly (may affect sound)

### Option 4: True Overlap-Add Synthesis (Proper Solution)
Use proper overlap-add windowing with:
- Grain size = N samples
- Hop size = N/2 samples (50% overlap)
- Grains triggered on aligned boundaries
- **Pros**: Mathematically correct, no discontinuities
- **Cons**: Requires refactoring grain trigger logic

### Option 5: Sample-Accurate State Preservation (Recommended)
Store grain states and verify continuity:
```cpp
// At end of buffer
for each grain:
    store finalAmplitude

// At start of next buffer  
for each grain:
    if |currentAmplitude - finalAmplitude| > threshold:
        apply micro-fade to smooth transition
```

## Next Steps

1. **Build and test** with new logging enabled
2. **Run audio through** with overlapping grains
3. **Check logs** for buffer boundary jumps
4. **Analyze** which grains are causing discontinuities
5. **Implement fix** based on findings

## Testing Commands

```bash
# Build with logging
./run.sh

# Check logs for clicks
tail -f ~/Library/Logs/dumumub-0000006.log | grep "BUFFER BOUNDARY CLICK"

# Analyze audio file for clicks
python3 analyze_clicks.py path/to/recording.wav 0.001
```
