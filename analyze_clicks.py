#!/usr/bin/env python3
"""
Audio Click Analyzer
Detects and analyzes discontinuities/clicks in WAV files from the granular synthesis plugin.
"""

import numpy as np
import matplotlib.pyplot as plt
from scipy.io import wavfile
from scipy import signal
import sys
import os

def analyze_clicks(wav_path, threshold=0.0005, context_samples=50):
    """
    Analyze a WAV file for click artifacts.
    
    Args:
        wav_path: Path to WAV file
        threshold: Minimum sample-to-sample jump to detect as click
        context_samples: Number of samples to show around each click
    """
    
    print(f"\n{'='*70}")
    print(f"Analyzing: {os.path.basename(wav_path)}")
    print(f"{'='*70}\n")
    
    # Load audio file
    sample_rate, audio = wavfile.read(wav_path)
    
    # Convert to float and normalize
    if audio.dtype == np.int16:
        audio = audio.astype(np.float32) / 32768.0
    elif audio.dtype == np.int32:
        audio = audio.astype(np.float32) / 2147483648.0
    
    # Handle stereo - analyze left channel
    if len(audio.shape) > 1:
        print(f"Stereo file detected - analyzing left channel")
        audio = audio[:, 0]
    
    print(f"Sample Rate: {sample_rate} Hz")
    print(f"Duration: {len(audio)/sample_rate:.2f} seconds")
    print(f"Total Samples: {len(audio)}")
    print(f"Click Detection Threshold: {threshold}")
    print(f"Buffer Size (assumed): 512 samples\n")
    
    # Calculate sample-to-sample differences
    diffs = np.abs(np.diff(audio))
    
    # Find clicks (discontinuities above threshold)
    click_indices = np.where(diffs > threshold)[0]
    
    print(f"{'='*70}")
    print(f"CLICK DETECTION RESULTS")
    print(f"{'='*70}\n")
    print(f"Total Clicks Found: {len(click_indices)}")
    
    if len(click_indices) == 0:
        print("\n✓ No clicks detected above threshold!")
        return
    
    # Statistics
    click_magnitudes = diffs[click_indices]
    print(f"Largest Click: {np.max(click_magnitudes):.6f}")
    print(f"Smallest Click: {np.min(click_magnitudes):.6f}")
    print(f"Average Click: {np.mean(click_magnitudes):.6f}")
    print(f"Median Click: {np.median(click_magnitudes):.6f}\n")
    
    # Check buffer boundary alignment (512 samples)
    buffer_size = 512
    buffer_aligned = 0
    for idx in click_indices:
        if idx % buffer_size == 0 or (idx + 1) % buffer_size == 0:
            buffer_aligned += 1
    
    print(f"Clicks at Buffer Boundaries (512): {buffer_aligned}/{len(click_indices)} ({100*buffer_aligned/len(click_indices):.1f}%)")
    
    # Show detailed info for top 20 clicks
    print(f"\n{'='*70}")
    print(f"TOP 20 LARGEST CLICKS")
    print(f"{'='*70}\n")
    print(f"{'Rank':<6} {'Sample':<10} {'Time (s)':<12} {'Magnitude':<12} {'From → To':<25} {'Buffer?'}")
    print(f"{'-'*70}")
    
    # Sort by magnitude
    sorted_indices = click_indices[np.argsort(click_magnitudes)[::-1]]
    sorted_magnitudes = np.sort(click_magnitudes)[::-1]
    
    for i, (idx, mag) in enumerate(zip(sorted_indices[:20], sorted_magnitudes[:20])):
        time_sec = idx / sample_rate
        from_val = audio[idx]
        to_val = audio[idx + 1]
        
        # Check if at buffer boundary
        at_buffer = "YES" if (idx % buffer_size == 0 or (idx + 1) % buffer_size == 0) else "no"
        buffer_num = idx // buffer_size
        sample_in_buffer = idx % buffer_size
        
        print(f"{i+1:<6} {idx:<10} {time_sec:<12.6f} {mag:<12.6f} {from_val:>8.6f} → {to_val:<8.6f}  {at_buffer}")
    
    # Analyze frequency content around clicks
    print(f"\n{'='*70}")
    print(f"FREQUENCY ANALYSIS")
    print(f"{'='*70}\n")
    
    # Take a few clicks and analyze their spectral content
    for i, idx in enumerate(sorted_indices[:5]):
        if i >= 3:  # Limit to first 3 for brevity
            break
            
        # Extract context around click
        start = max(0, idx - context_samples)
        end = min(len(audio), idx + context_samples + 1)
        context = audio[start:end]
        
        # FFT analysis
        if len(context) > 0:
            fft = np.fft.rfft(context)
            freqs = np.fft.rfftfreq(len(context), 1/sample_rate)
            magnitudes = np.abs(fft)
            
            # Find dominant frequencies
            top_freq_indices = np.argsort(magnitudes)[::-1][:5]
            
            print(f"Click #{i+1} at sample {idx} ({idx/sample_rate:.6f}s):")
            print(f"  Top frequencies:")
            for j, freq_idx in enumerate(top_freq_indices):
                if freq_idx < len(freqs):
                    print(f"    {j+1}. {freqs[freq_idx]:.1f} Hz (magnitude: {magnitudes[freq_idx]:.4f})")
            print()
    
    # Create visualization
    create_click_visualization(audio, sample_rate, click_indices, sorted_indices[:10], 
                              threshold, wav_path)

def create_click_visualization(audio, sample_rate, all_clicks, top_clicks, threshold, wav_path):
    """Create a visualization of the audio with clicks highlighted."""
    
    fig, axes = plt.subplots(3, 1, figsize=(14, 10))
    
    # Plot 1: Full waveform with clicks marked
    time = np.arange(len(audio)) / sample_rate
    axes[0].plot(time, audio, linewidth=0.5, alpha=0.7)
    axes[0].scatter(all_clicks / sample_rate, audio[all_clicks], 
                   color='red', s=20, alpha=0.6, label=f'Clicks (n={len(all_clicks)})')
    axes[0].set_title(f'Full Waveform - {os.path.basename(wav_path)}')
    axes[0].set_xlabel('Time (seconds)')
    axes[0].set_ylabel('Amplitude')
    axes[0].legend()
    axes[0].grid(True, alpha=0.3)
    
    # Plot 2: Sample-to-sample differences
    diffs = np.abs(np.diff(audio))
    axes[1].plot(diffs, linewidth=0.5, alpha=0.7)
    axes[1].axhline(y=threshold, color='r', linestyle='--', label=f'Threshold ({threshold})')
    axes[1].scatter(all_clicks, diffs[all_clicks], color='red', s=20, alpha=0.6)
    axes[1].set_title('Sample-to-Sample Jump Magnitude')
    axes[1].set_xlabel('Sample Index')
    axes[1].set_ylabel('|Difference|')
    axes[1].set_yscale('log')
    axes[1].legend()
    axes[1].grid(True, alpha=0.3)
    
    # Plot 3: Zoomed view of largest click
    if len(top_clicks) > 0:
        largest_click = top_clicks[0]
        context = 200  # samples to show around click
        start = max(0, largest_click - context)
        end = min(len(audio), largest_click + context)
        
        zoom_time = np.arange(start, end) / sample_rate
        zoom_audio = audio[start:end]
        
        axes[2].plot(zoom_time, zoom_audio, linewidth=1, marker='o', markersize=3)
        axes[2].axvline(x=largest_click/sample_rate, color='r', linestyle='--', 
                       label=f'Click at sample {largest_click}')
        axes[2].set_title(f'Zoomed View: Largest Click (±{context} samples)')
        axes[2].set_xlabel('Time (seconds)')
        axes[2].set_ylabel('Amplitude')
        axes[2].legend()
        axes[2].grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    # Save figure
    output_path = wav_path.replace('.wav', '_click_analysis.png')
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"\n{'='*70}")
    print(f"Visualization saved to: {output_path}")
    print(f"{'='*70}\n")
    
    # Don't show interactively, just save
    plt.close()

def main():
    if len(sys.argv) < 2:
        print("Usage: python analyze_clicks.py <audio_file.wav> [threshold]")
        print("\nExample: python analyze_clicks.py recording.wav 0.001")
        sys.exit(1)
    
    wav_path = sys.argv[1]
    threshold = float(sys.argv[2]) if len(sys.argv) > 2 else 0.001
    
    if not os.path.exists(wav_path):
        print(f"Error: File not found: {wav_path}")
        sys.exit(1)
    
    analyze_clicks(wav_path, threshold)

if __name__ == "__main__":
    main()
