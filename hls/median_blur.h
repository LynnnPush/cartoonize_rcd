#ifndef MEDIAN_BLUR_HPP
#define MEDIAN_BLUR_HPP

#include <ap_int.h>     
#include <stdint.h>

#include "pixel_types.hpp"

// Constants
#define K_SIZE 5    
// Kernel size for median blur, affects batcher sort implementation. Check batcher_sort_32 if you want to change this.
#define WIDTH 1280
#define HEIGHT 720
#define NUM_ELEMENTS (K_SIZE * K_SIZE)

// Macros for RGB/Grayscale extraction
#define rgba2r(v) ((v)&0xFF)
#define rgba2g(v) (((v)&0xFF00) >> 8)
#define rgba2b(v) (((v)&0xFF0000) >> 16)
#define rgba2a(v) (((v)&0xFF000000) >> 24)

#define r2rgba(v) ((v)&0xFF)
#define g2rgba(v) (((v)&0xFF) << 8)
#define b2rgba(v) (((v)&0xFF) << 16)
#define a2rgba(v) (((v)&0xFF) << 24)

// 3. Function Prototype
void hls_bubble_sort(uint8_t input_arr[NUM_ELEMENTS], uint8_t &median);
void median_blur(pixel_stream &src, pixel_stream &dst);

// =============================================================================
// Macro for compare-and-swap (the fundamental sorting network operation)
// =============================================================================
#define CMP_SWAP(arr, i, j) \
    do { \
        if (arr[i] > arr[j]) { \
            uint8_t t = arr[i]; \
            arr[i] = arr[j]; \
            arr[j] = t; \
        } \
    } while(0)

// =============================================================================
// Batcher's Odd-Even Merge Sort for 32 elements
// For 25-element median (determined by K_SIZE above), we pad to 32 (next power of 2)
// Total comparators: 191 (vs 300 for bubble sort)
// Depth: 15 stages (vs 24 for bubble sort)
// Parallelizable each step within each phase (as no data dependency), where HLS is supposed to analyze and compile accordingly (Verify synthesis results) 
// =============================================================================
inline void batcher_sort_32(uint8_t arr[32]) {
    #pragma HLS INLINE
    #pragma HLS ARRAY_PARTITION variable=arr complete

    // =========================================================================
    // PHASE 1: Sort adjacent pairs (16 comparators, 1 stage)
    // =========================================================================
    CMP_SWAP(arr,0,1);   CMP_SWAP(arr,2,3);   CMP_SWAP(arr,4,5);   CMP_SWAP(arr,6,7);
    CMP_SWAP(arr,8,9);   CMP_SWAP(arr,10,11); CMP_SWAP(arr,12,13); CMP_SWAP(arr,14,15);
    CMP_SWAP(arr,16,17); CMP_SWAP(arr,18,19); CMP_SWAP(arr,20,21); CMP_SWAP(arr,22,23);
    CMP_SWAP(arr,24,25); CMP_SWAP(arr,26,27); CMP_SWAP(arr,28,29); CMP_SWAP(arr,30,31);
    // Subtotal: 16

    // =========================================================================
    // PHASE 2: Merge pairs into 4-element sorted sequences
    // 8 merges × M(4)=3 = 24 comparators, 2 stages
    // =========================================================================
    // Step 2a: Compare at distance 2 (16 comparators)
    CMP_SWAP(arr,0,2);   CMP_SWAP(arr,1,3);   CMP_SWAP(arr,4,6);   CMP_SWAP(arr,5,7);
    CMP_SWAP(arr,8,10);  CMP_SWAP(arr,9,11);  CMP_SWAP(arr,12,14); CMP_SWAP(arr,13,15);
    CMP_SWAP(arr,16,18); CMP_SWAP(arr,17,19); CMP_SWAP(arr,20,22); CMP_SWAP(arr,21,23);
    CMP_SWAP(arr,24,26); CMP_SWAP(arr,25,27); CMP_SWAP(arr,28,30); CMP_SWAP(arr,29,31);
    // Step 2b: Fix adjacent pairs (8 comparators)
    CMP_SWAP(arr,1,2);   CMP_SWAP(arr,5,6);   CMP_SWAP(arr,9,10);  CMP_SWAP(arr,13,14);
    CMP_SWAP(arr,17,18); CMP_SWAP(arr,21,22); CMP_SWAP(arr,25,26); CMP_SWAP(arr,29,30);
    // Subtotal: 24, Running total: 40

    // =========================================================================
    // PHASE 3: Merge 4-element into 8-element sorted sequences  
    // 4 merges × M(8)=9 = 36 comparators, 3 stages
    // =========================================================================
    // Step 3a: Compare at distance 4 (16 comparators)
    CMP_SWAP(arr,0,4);   CMP_SWAP(arr,1,5);   CMP_SWAP(arr,2,6);   CMP_SWAP(arr,3,7);
    CMP_SWAP(arr,8,12);  CMP_SWAP(arr,9,13);  CMP_SWAP(arr,10,14); CMP_SWAP(arr,11,15);
    CMP_SWAP(arr,16,20); CMP_SWAP(arr,17,21); CMP_SWAP(arr,18,22); CMP_SWAP(arr,19,23);
    CMP_SWAP(arr,24,28); CMP_SWAP(arr,25,29); CMP_SWAP(arr,26,30); CMP_SWAP(arr,27,31);
    // Step 3b: Recursive odd-even merge - compare at distance 2 (8 comparators)
    CMP_SWAP(arr,2,4);   CMP_SWAP(arr,3,5);   CMP_SWAP(arr,10,12); CMP_SWAP(arr,11,13);
    CMP_SWAP(arr,18,20); CMP_SWAP(arr,19,21); CMP_SWAP(arr,26,28); CMP_SWAP(arr,27,29);
    // Step 3c: Fix adjacent pairs (12 comparators)
    CMP_SWAP(arr,1,2);   CMP_SWAP(arr,3,4);   CMP_SWAP(arr,5,6);
    CMP_SWAP(arr,9,10);  CMP_SWAP(arr,11,12); CMP_SWAP(arr,13,14);
    CMP_SWAP(arr,17,18); CMP_SWAP(arr,19,20); CMP_SWAP(arr,21,22);
    CMP_SWAP(arr,25,26); CMP_SWAP(arr,27,28); CMP_SWAP(arr,29,30);
    // Subtotal: 36, Running total: 76

    // =========================================================================
    // PHASE 4: Merge 8-element into 16-element sorted sequences
    // 2 merges × M(16)=25 = 50 comparators, 4 stages
    // =========================================================================
    // Step 4a: Compare at distance 8 (16 comparators)
    CMP_SWAP(arr,0,8);   CMP_SWAP(arr,1,9);   CMP_SWAP(arr,2,10);  CMP_SWAP(arr,3,11);
    CMP_SWAP(arr,4,12);  CMP_SWAP(arr,5,13);  CMP_SWAP(arr,6,14);  CMP_SWAP(arr,7,15);
    CMP_SWAP(arr,16,24); CMP_SWAP(arr,17,25); CMP_SWAP(arr,18,26); CMP_SWAP(arr,19,27);
    CMP_SWAP(arr,20,28); CMP_SWAP(arr,21,29); CMP_SWAP(arr,22,30); CMP_SWAP(arr,23,31);
    // Step 4b: Compare at distance 4 (8 comparators)
    CMP_SWAP(arr,4,8);   CMP_SWAP(arr,5,9);   CMP_SWAP(arr,6,10);  CMP_SWAP(arr,7,11);
    CMP_SWAP(arr,20,24); CMP_SWAP(arr,21,25); CMP_SWAP(arr,22,26); CMP_SWAP(arr,23,27);
    // Step 4c: Compare at distance 2 (12 comparators)
    CMP_SWAP(arr,2,4);   CMP_SWAP(arr,3,5);   CMP_SWAP(arr,6,8);   CMP_SWAP(arr,7,9);
    CMP_SWAP(arr,10,12); CMP_SWAP(arr,11,13);
    CMP_SWAP(arr,18,20); CMP_SWAP(arr,19,21); CMP_SWAP(arr,22,24); CMP_SWAP(arr,23,25);
    CMP_SWAP(arr,26,28); CMP_SWAP(arr,27,29);
    // Step 4d: Fix adjacent pairs (18 comparators)
    CMP_SWAP(arr,1,2);   CMP_SWAP(arr,3,4);   CMP_SWAP(arr,5,6);   CMP_SWAP(arr,7,8);
    CMP_SWAP(arr,9,10);  CMP_SWAP(arr,11,12); CMP_SWAP(arr,13,14);
    CMP_SWAP(arr,17,18); CMP_SWAP(arr,19,20); CMP_SWAP(arr,21,22); CMP_SWAP(arr,23,24);
    CMP_SWAP(arr,25,26); CMP_SWAP(arr,27,28); CMP_SWAP(arr,29,30);
    // Subtotal: 50, Running total: 126

    // =========================================================================
    // PHASE 5: Final merge into 32-element sorted sequence
    // 1 merge × M(32)=65 = 65 comparators, 5 stages
    // =========================================================================
    // Step 5a: Compare at distance 16 (16 comparators)
    CMP_SWAP(arr,0,16);  CMP_SWAP(arr,1,17);  CMP_SWAP(arr,2,18);  CMP_SWAP(arr,3,19);
    CMP_SWAP(arr,4,20);  CMP_SWAP(arr,5,21);  CMP_SWAP(arr,6,22);  CMP_SWAP(arr,7,23);
    CMP_SWAP(arr,8,24);  CMP_SWAP(arr,9,25);  CMP_SWAP(arr,10,26); CMP_SWAP(arr,11,27);
    CMP_SWAP(arr,12,28); CMP_SWAP(arr,13,29); CMP_SWAP(arr,14,30); CMP_SWAP(arr,15,31);
    // Step 5b: Compare at distance 8 (8 comparators)
    CMP_SWAP(arr,8,16);  CMP_SWAP(arr,9,17);  CMP_SWAP(arr,10,18); CMP_SWAP(arr,11,19);
    CMP_SWAP(arr,12,20); CMP_SWAP(arr,13,21); CMP_SWAP(arr,14,22); CMP_SWAP(arr,15,23);
    // Step 5c: Compare at distance 4 (12 comparators)
    CMP_SWAP(arr,4,8);   CMP_SWAP(arr,5,9);   CMP_SWAP(arr,6,10);  CMP_SWAP(arr,7,11);
    CMP_SWAP(arr,12,16); CMP_SWAP(arr,13,17); CMP_SWAP(arr,14,18); CMP_SWAP(arr,15,19);
    CMP_SWAP(arr,20,24); CMP_SWAP(arr,21,25); CMP_SWAP(arr,22,26); CMP_SWAP(arr,23,27);
    // Step 5d: Compare at distance 2 (14 comparators)
    CMP_SWAP(arr,2,4);   CMP_SWAP(arr,3,5);   CMP_SWAP(arr,6,8);   CMP_SWAP(arr,7,9);
    CMP_SWAP(arr,10,12); CMP_SWAP(arr,11,13); CMP_SWAP(arr,14,16); CMP_SWAP(arr,15,17);
    CMP_SWAP(arr,18,20); CMP_SWAP(arr,19,21); CMP_SWAP(arr,22,24); CMP_SWAP(arr,23,25);
    CMP_SWAP(arr,26,28); CMP_SWAP(arr,27,29);
    // Step 5e: Fix adjacent pairs (15 comparators)
    CMP_SWAP(arr,1,2);   CMP_SWAP(arr,3,4);   CMP_SWAP(arr,5,6);   CMP_SWAP(arr,7,8);
    CMP_SWAP(arr,9,10);  CMP_SWAP(arr,11,12); CMP_SWAP(arr,13,14); CMP_SWAP(arr,15,16);
    CMP_SWAP(arr,17,18); CMP_SWAP(arr,19,20); CMP_SWAP(arr,21,22); CMP_SWAP(arr,23,24);
    CMP_SWAP(arr,25,26); CMP_SWAP(arr,27,28); CMP_SWAP(arr,29,30);
    // Subtotal: 65, Running total: 191
}

// =============================================================================
// Main median function using sorting network
// Drop-in replacement for hls_bubble_sort
// =============================================================================
void hls_oddeven_sort_median(uint8_t input_arr[25], uint8_t &median);


#endif
