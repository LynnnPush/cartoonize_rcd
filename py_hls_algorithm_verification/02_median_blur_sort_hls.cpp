#include <ap_int.h> # If using Xilinx arbitrary precision types, else use int

// Define the window size (e.g., for ksize=3, WINDOW_SIZE=9)
#define K_SIZE 3
#define WINDOW_SIZE (K_SIZE * K_SIZE)

unsigned char median_filter_kernel(unsigned char window_array[WINDOW_SIZE]) {
    #pragma HLS PIPELINE
    
    // 1. Temporary buffer to perform sorting so we don't modify the input directly
    unsigned char sorted_window[WINDOW_SIZE];
    
    // Copy input to temp
    for(int i = 0; i < WINDOW_SIZE; i++) {
        sorted_window[i] = window_array[i];
    }

    // 2. Bubble Sort (Optimized for HLS: Fixed iterations)
    // The synthesizer can unroll these loops to generate comparator hardware.
    for(int i = 0; i < WINDOW_SIZE - 1; i++) {
        for(int j = 0; j < WINDOW_SIZE - i - 1; j++) {
            if(sorted_window[j] > sorted_window[j + 1]) {
                // Swap logic
                unsigned char temp = sorted_window[j];
                sorted_window[j] = sorted_window[j + 1];
                sorted_window[j + 1] = temp;
            }
        }
    }

    // 3. Pick the median
    // For 9 elements, index 4 is the median (0,1,2,3, [4], 5,6,7,8)
    return sorted_window[WINDOW_SIZE / 2];
}