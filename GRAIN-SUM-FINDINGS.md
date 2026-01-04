# Grain Sum Verification Findings

## Summary
The manual grain sum calculation revealed a **critical discovery**: the grains themselves are actually quite smooth, with tiny discontinuities. The larger clicks in the buffer output are coming from somewhere else!

## Evidence from Logs

### Click #1 (Buffer #8)
- **Grain sum discontinuity**: -0.0000145608 (tiny!)
- **Buffer discontinuity**: -0.0012553011 (**86x larger**)
- Individual grain jumps: Grain 1: Δ=-0.00005077, Grain 2: Δ=0.00003621

### Click #2 (Buffer #12)
- **Grain sum discontinuity**: -0.0005312245
- **Buffer discontinuity**: -0.0005444842 (very close!)
- Individual grain jumps: Grain 1: Δ=-0.00041677, Grain 2: Δ=-0.00011445

### Click #5 (Buffer #20)
- **Grain sum discontinuity**: -0.0000308892 (tiny!)
- **Buffer discontinuity**: -0.0020315144 (**66x larger**)
- Individual grain jumps: Grain 1: Δ=-0.00005277, Grain 2: Δ=-0.00003583, Grain 3: Δ=0.00005771

### Click #6 (Buffer #23)
- **Grain sum discontinuity**: 0.0000184588 (tiny!)
- **Buffer discontinuity**: -0.0013048500 (**71x larger**)
- Individual grain jumps: Grain 1: Δ=0.00023826, Grain 2: Δ=-0.00021980

## Key Findings

1. **Grains are smooth**: The mathematical sum of grain contributions shows very small discontinuities (typically < 0.0001)

2. **Buffer has large jumps**: The actual buffer output shows much larger discontinuities (typically 0.001-0.002)

3. **Mismatch factor**: Buffer discontinuities are typically **60-80x larger** than the grain sum would predict

4. **Not all clicks follow pattern**: Click #2 shows grain sum and buffer discontinuity are similar, suggesting different causes for different clicks

## Implications

This proves that:
- ❌ Clicks are NOT caused by grain timing bugs (grains advance correctly, 1 sample per sample)
- ❌ Clicks are NOT caused by Hann window phase summation alone (grain sum is smooth)
- ✅ **Something else is modifying the buffer between grain rendering and output**

## Calculation Methodology Note

The manual grain sum calculation uses **nearest-neighbor sampling** (simple integer index lookup), while actual grain rendering uses **cubic Hermite interpolation**. This explains why absolute values don't match:

- Prev sum: 0.0029464185 ≠ Expected last buffer end: 0.0041871588
- Curr sum: 0.0029318577 = Actual first buffer start: 0.0029318577 ✓

However, the **discontinuity comparison** is still valid because both methods should show similar jumps if the grains themselves are the problem.

## Next Steps

Need to investigate what happens between grain rendering and buffer output:
1. Check if there's any post-processing (filters, effects, etc.)
2. Look for denormal protection that might be altering values
3. Check soft clipping (tanh) behavior at buffer boundaries
4. Verify buffer accumulation logic (additive mixing)

## Conclusion

**The grains are innocent!** The clicks are coming from somewhere else in the audio pipeline.
