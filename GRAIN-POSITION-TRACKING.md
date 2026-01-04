# Grain Position Tracking Investigation

## The Question
Do grains advance correctly between buffers, or is there a >1 sample jump?

## Current Understanding

### Buffer N (last sample):
```
grainPosition = 100 (from grain.playbackPosition)
i = 511 (last sample in buffer)
Amplitude calculated for: grainPosition + i = 100 + 511 = 611
After buffer: grain.playbackPosition += 512 → grain.playbackPosition = 612
```

### Buffer N+1 (first sample):
```
grainPosition = 612 (NEW position after updateGrains)
i = 0 (first sample in buffer)
Amplitude calculated for: grainPosition + i = 612 + 0 = 612
```

### Analysis
- Last sample of buffer N: position 611
- First sample of buffer N+1: position 612
- **Difference: 1 sample** ✓ CORRECT!

## So What's Causing the Clicks?

If grain advancement is correct (only 1 sample jump), then the clicks must be from:

1. **Multiple grains with different Hann phases overlapping** - Each grain has a different playback position, so their Hann windows are at different phases. When summed, the total can have discontinuities at buffer boundaries.

2. **Natural amplitude changes in the Hann window** - As explained in WHY-CLICKS-HAPPEN-EXPLAINED.md, the Hann window creates different amplitude slopes for each grain.

## Next Steps

Need to add precise logging to verify:
- [ ] Log the EXACT amplitude of the final output (sum of all grains) for the last sample of buffer N
- [ ] Log the EXACT amplitude of the final output (sum of all grains) for the first sample of buffer N+1
- [ ] Verify the jump is > 0.0003 (crossfade threshold)
- [ ] Track which grains are active and their exact positions

This will confirm whether:
- The discontinuity is from grain advancement (unlikely based on code analysis)
- The discontinuity is from natural Hann window summation (likely - already explained in docs)
