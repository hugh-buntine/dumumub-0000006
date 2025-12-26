# 🐛 CLICK BUG IDENTIFIED

## Summary
**FOUND THE BUG!** Every grain skips its first 512 samples (the fade-in), causing clicks.

## Evidence from Logs

### Buffer Size
```
prepareToPlay called - Buffer Size: 512 samples
```

### Grain Lifecycle
```
GRAIN TRIGGER #1: currentGrains=0
PROCESSING GRAIN #1: position=512, totalSize=12877
```

**The grain jumps from 0 → 512 before being rendered!**

### Grain Completion
```
GRAIN COMPLETED: finalPos=13312, grainSize=12877 (pos went past size by 435)
```
- Should end at: 12877
- Actually ends at: 13312  
- Difference: **435 samples** (which is `512 - 77` approximately)

## Root Cause

### Code Location
`PluginProcessor.cpp` lines 638-640:

```cpp
// Line 638: Trigger new grain
particle->triggerNewGrain (audioFileBuffer.getNumSamples());

// Line 640: Update ALL grains (including the one just created!)
particle->updateGrains (buffer.getNumSamples());  // +512 samples

// Later: Process grains (first grain now at position 512!)
```

### The Problem Flow

1. **Grain Created** (position = 0)
   ```
   activeGrains.push_back(Grain(startSample))
   // grain.playbackPosition = 0
   ```

2. **Grain Updated** (position = 0 + 512 = 512)
   ```cpp
   for (auto& grain : activeGrains)
       grain.playbackPosition += numSamples;  // numSamples = 512
   ```

3. **Grain Rendered** (starts at position 512!)
   ```cpp
   int grainPosition = grain.playbackPosition;  // = 512, NOT 0!
   ```

### What This Means

**Hann Window Values:**
- Position 0: amplitude ≈ 0.0000 (silent start, no click)
- Position 512: amplitude ≈ 0.0039 to 0.01+ (audible jump = CLICK!)

Even a 0.01 jump in one sample creates an audible pop/click.

## Why This Affects Every Grain

- **First grain**: Skips first 512 samples
- **Second grain**: Skips first 512 samples  
- **Third grain**: Skips first 512 samples
- **Every grain forever**: Skips fade-in!

## Why Frequency Doesn't Matter

Even at 5 Hz with grains NOT overlapping:
- Each grain still starts at position 512
- Each grain still has no fade-in
- Each grain still clicks

## The Fix (DO NOT IMPLEMENT YET - DIAGNOSTIC ONLY)

The fix would be to change the order in `processBlock()`:

### Current (WRONG):
```cpp
triggerNewGrain();
updateGrains();      // <-- This advances the NEW grain before rendering!
[render grains]
```

### Fixed (CORRECT):
```cpp
[render grains]      // <-- Render first (new grain not in list yet)
updateGrains();      // <-- Then advance for NEXT buffer
triggerNewGrain();   // <-- Create grain for NEXT buffer
```

OR keep the grain at position 0 during its first render:

```cpp
triggerNewGrain();
updateGrains();      // Only update OLD grains, not newly created ones
[render grains]
```

## Impact

- **Severity**: Critical - affects 100% of grains
- **Frequency**: Every grain, regardless of settings
- **Sound**: Poppy, high-frequency clicks
- **User Experience**: Unusable for smooth granular synthesis

## Validation

This explains:
- ✅ Why clicks happen on EVERY grain
- ✅ Why they're poppy/high-frequency (sudden amplitude jump)
- ✅ Why grain frequency doesn't matter
- ✅ Why even non-overlapping grains click
- ✅ Why no amplitude spike >0.5 was logged (jumps are ~0.01-0.05, not >0.5)
- ✅ Why ADSR doesn't matter (it's grain windowing, not envelope)
- ✅ Why grains complete 435 samples late (512 - some processing)

## Next Steps

User needs to decide on fix approach:
1. Reorder processBlock() to render before updating
2. Track new grains separately and skip first update
3. Start grains at negative position (-512) to compensate
4. Process grains before updating them

**Recommendation**: Option 1 or 2 (cleanest architecturally)
