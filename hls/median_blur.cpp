#include "median_blur.h"

// Bubble Sort to find the median in a KxK window, optimized for HLS
void hls_bubble_sort(uint8_t input_arr[NUM_ELEMENTS], uint8_t &median) {
    #pragma HLS INLINE

    // Local buffer to perform sorting so we don't modify the input directly
    uint8_t arr[NUM_ELEMENTS];
    #pragma HLS ARRAY_PARTITION variable=arr complete dim=0

    // Copy input to local buffer
    for(int i = 0; i < NUM_ELEMENTS; i++) {
        #pragma HLS UNROLL
        arr[i] = input_arr[i];
    }

    // Bubble Sort (Fully unrolled for HLS)
    for(int i = 0; i < NUM_ELEMENTS - 1; i++){
        #pragma HLS UNROLL
        for(int j = 0; j < NUM_ELEMENTS - i - 1; j++){
            #pragma HLS UNROLL
            if(arr[j] > arr[j + 1]){
                uint8_t temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }

    // Median is the middle element
    median = arr[NUM_ELEMENTS / 2];
}

void median_blur(pixel_stream &src, pixel_stream &dst){
    #pragma HLS PIPELINE II=1

    // Internal Buffers
    static uint16_t x = 0;
    static uint16_t y = 0;
    static uint8_t cnt = 0; // Cyclic buffer index

    // Line Buffer: K-1 rows
    static uint8_t line_buffer[K_SIZE - 1][WIDTH];
    #pragma HLS ARRAY_PARTITION variable=line_buffer dim=1 complete
    #pragma HLS BIND_STORAGE variable=line_buffer type=ram_2p impl=bram // FORCE the implementation to BRAM (RAM_2P or RAM_S2P)
    #pragma HLS DEPENDENCE variable=line_buffer inter false

    // Window Buffer: KxK pixel 
    static uint8_t window_buffer[K_SIZE][K_SIZE];
    #pragma HLS ARRAY_PARTITION variable=window_buffer complete dim=0

    // Stream handling
    pixel_data p_in, p_out;

    // Read input pixel
    src >> p_in;

    // Handle frame start
    if (p_in.user) {
        x = 0;
        y = 0;
        cnt = 0;
    }

    uint8_t new_pixel = rgba2r(p_in.data); // Assuming grayscale in R channel

    // ----------------------------------------------------------------------
    // 1. Update Line Buffer and Window Buffer
    // ----------------------------------------------------------------------

    // Shift window left to make toom for new column
    for (int i = 0; i < K_SIZE; i++) {
        #pragma HLS UNROLL
        for (int j = 0; j < K_SIZE - 1; j++) {
            #pragma HLS UNROLL
            window_buffer[i][j] = window_buffer[i][j + 1];
        }
    }
    
    
    if (x < WIDTH) {
        uint8_t col_bank[K_SIZE - 1];
        #pragma HLS ARRAY_PARTITION variable=col_bank complete dim=0

        for (int r = 0; r < K_SIZE - 1; r++) {
            #pragma HLS UNROLL
            col_bank[r] = line_buffer[r][x];
        }

        for (int i = 0; i < K_SIZE - 1; i++) {
            #pragma HLS UNROLL
            int idx = cnt + i;
            if (idx >= (K_SIZE - 1)) idx -= (K_SIZE - 1);
            window_buffer[i][K_SIZE - 1] = col_bank[idx];
        }

        window_buffer[K_SIZE - 1][K_SIZE - 1] = new_pixel;
        line_buffer[cnt][x] = new_pixel;
    }

    // ----------------------------------------------------------------------
    // 2. Median Calculation
    // ----------------------------------------------------------------------

    // We can only compute median once we have filled the buffer enough
    if (y >= K_SIZE - 1 && x >= K_SIZE - 1) {
        // Flatten window for sorting
        uint8_t flat_window[NUM_ELEMENTS];
        #pragma HLS ARRAY_PARTITION variable=flat_window complete dim=0

        int idx = 0;
        for(int i = 0; i < K_SIZE; i++) {
            #pragma HLS UNROLL
            for(int j = 0; j < K_SIZE; j++) {
                #pragma HLS UNROLL
                flat_window[idx++] = window_buffer[i][j];
            }
        }

        // Compute median using bubble sort
        uint8_t median_val;
        hls_bubble_sort(flat_window, median_val);

        // Prepare output pixel
        p_out.data = r2rgba(median_val) | g2rgba(median_val) | b2rgba(median_val);
    }
    else {
        // Not enough data yet, fall back to pass-through pixel to avoid black borders
        p_out.data = r2rgba(new_pixel) | g2rgba(new_pixel) | b2rgba(new_pixel);
    }

    // Write output pixel metadata and update counters
    p_out.user = p_in.user;
    p_out.last = p_in.last;

    dst << p_out;

    if (p_in.last) {
        x = 0;
        y++;
        cnt++;
        if (cnt >= (K_SIZE - 1)) cnt = 0;
    } 
    else {
        x++;
    }
}

// Optional standalone stream wrapper for testing this stage only
void median_blur_stream(pixel_stream &src, pixel_stream &dst, int frame)
{
    (void)frame;
    median_blur(src, dst);
}
