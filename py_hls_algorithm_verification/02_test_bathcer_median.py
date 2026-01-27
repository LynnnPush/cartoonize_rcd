# test_bathcer_median.py
import re
import random
from pathlib import Path

HLS_HEADER_PATH = (Path(__file__).resolve().parent / ".." / "hls" / "median_blur.h").resolve()

def load_cmp_swaps(path: Path):
    """Extract CMP_SWAP pairs from batcher_sort_32 in the header file."""
    text = path.read_text()
    m = re.search(r'inline void batcher_sort_32\(uint8_t arr\[32\]\)\s*\{(.*?)\n\}', text, re.S)
    if not m:
        raise RuntimeError("batcher_sort_32 not found in header")

    body = m.group(1)
    pairs = []
    for line in body.splitlines():
        for a, b in re.findall(r"CMP_SWAP\(arr,(\d+),(\d+)\)", line):
            pairs.append((int(a), int(b)))
    return pairs

def hls_sort(arr32, pairs):
    """Apply HLS sorting network to a 32-element array."""
    arr = list(arr32)
    for i, j in pairs:
        if arr[i] > arr[j]:
            arr[i], arr[j] = arr[j], arr[i]
    return arr

def hls_median_25(arr25, pairs):
    """Get median of 25 elements using HLS sorting network (pad to 32)."""
    padded = list(arr25) + [255] * 7  # Pad with 255 (sorts to end)
    sorted_arr = hls_sort(padded, pairs)
    return sorted_arr[12], sorted_arr  # Median at index 12 for 25 elements

def standard_median_25(arr25):
    """Get median of 25 elements using Python standard sort."""
    sorted_arr = sorted(arr25)
    return sorted_arr[12], sorted_arr  # Median at index 12 for 25 elements

def main():
    # Load sorting network from header
    if not HLS_HEADER_PATH.exists():
        print(f"Error: Header file not found at {HLS_HEADER_PATH}")
        print("Please update HLS_HEADER_PATH in the script.")
        return 1
    
    pairs = load_cmp_swaps(HLS_HEADER_PATH)
    print(f"Loaded {len(pairs)} comparators from {HLS_HEADER_PATH}\n")
    
    # Generate random input
    input_arr = [random.randint(0, 255) for _ in range(25)]
    
    # Print input
    print("=" * 70)
    print("INPUT (25 elements):")
    print("=" * 70)
    print(input_arr)
    print()
    
    # HLS sorting network result
    hls_median, hls_sorted = hls_median_25(input_arr, pairs)
    
    # Standard Python sort result
    std_median, std_sorted = standard_median_25(input_arr)
    
    # Print results
    print("=" * 70)
    print("RESULTS:")
    print("=" * 70)
    print(f"Standard sorted: {std_sorted}")
    print(f"HLS sorted:      {hls_sorted[:25]}")  # Only show first 25 (exclude padding)
    print()
    print(f"Standard median (index 12): {std_median}")
    print(f"HLS median (index 12):      {hls_median}")
    print()
    
    # Compare
    if hls_median == std_median:
        print("✓ PASS: Medians match!")
        
        # Also verify full sort is correct
        if hls_sorted[:25] == std_sorted:
            print("✓ PASS: Full sorted arrays match!")
        else:
            print("✗ FAIL: Sorted arrays differ (but medians match)")
            # Find first difference
            for i in range(25):
                if hls_sorted[i] != std_sorted[i]:
                    print(f"  First difference at index {i}: HLS={hls_sorted[i]}, std={std_sorted[i]}")
                    break
        return 0
    else:
        print("✗ FAIL: Medians DO NOT match!")
        print()
        # Show where sorting went wrong
        print("Differences in sorted output:")
        for i in range(25):
            if hls_sorted[i] != std_sorted[i]:
                print(f"  Index {i}: HLS={hls_sorted[i]}, std={std_sorted[i]}")
        return 1

if __name__ == "__main__":
    exit(main())