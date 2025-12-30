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

void hls_oddeven_sort_median(uint8_t input_arr[25], uint8_t &median) {
    #pragma HLS INLINE
    
    // Pad 25 elements to 32 (next power of 2)
    uint8_t padded[32];
    #pragma HLS ARRAY_PARTITION variable=padded complete
    
    // Copy input and pad with maximum value (will sort to end)
    for(int i = 0; i < 25; i++) {
        #pragma HLS UNROLL
        padded[i] = input_arr[i];
    }
    for(int i = 25; i < 32; i++) {
        #pragma HLS UNROLL
        padded[i] = 255; // Sentinel values sort to the end
    }
    
    // Apply sorting network
    batcher_sort_32(padded);
    
    // Median of 25 elements is at index 12 (0-indexed middle)
    median = padded[12];
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
    // 1. Shift window left
    // ----------------------------------------------------------------------
    for (int i = 0; i < K_SIZE; i++) {
        #pragma HLS UNROLL
        for (int j = 0; j < K_SIZE - 1; j++) {
            #pragma HLS UNROLL
            window_buffer[i][j] = window_buffer[i][j + 1];
        }
    }

    // ----------------------------------------------------------------------
    // 2. Fill rightmost column with edge replication for top border
    //    (HLS-friendly: simplified conditionals using ternary MUX)
    // ----------------------------------------------------------------------
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
            
            // Calculate which actual image row this window position corresponds to
            int src_row = (int)y - (K_SIZE - 1) + i;
            
            // HLS-friendly: Use simple ternary MUX instead of nested if-else
            uint8_t selected_pixel;
            if (src_row < 0) {
                // Row doesn't exist - replicate edge
                selected_pixel = (y == 0) ? new_pixel : col_bank[0];
            } else {
                // Row exists - use it normally
                selected_pixel = col_bank[idx];
            }
            window_buffer[i][K_SIZE - 1] = selected_pixel;
        }

        window_buffer[K_SIZE - 1][K_SIZE - 1] = new_pixel;
        line_buffer[cnt][x] = new_pixel;
    }

    // ----------------------------------------------------------------------
    // 3. Edge replication for left border
    //    (HLS-friendly: fixed loop bounds, no outer conditional, MUX-based)
    // ----------------------------------------------------------------------
    
    // Pre-calculate edge values BEFORE the loop (cleaner for HLS)
    uint8_t edge_vals[K_SIZE];
    #pragma HLS ARRAY_PARTITION variable=edge_vals complete dim=0
    for (int i = 0; i < K_SIZE; i++) {
        #pragma HLS UNROLL
        edge_vals[i] = window_buffer[i][K_SIZE - 1];
    }
    
    // Fixed loop bounds with conditional assignment inside (MUX-based)
    // No outer "if (x < K_SIZE - 1)" wrapper - the condition is inside
    for (int i = 0; i < K_SIZE; i++) {
        #pragma HLS UNROLL
        for (int j = 0; j < K_SIZE - 1; j++) {
            #pragma HLS UNROLL
            if ((int)x < (K_SIZE - 1 - j)) {
                window_buffer[i][j] = edge_vals[i];
            }
            // else: keep the shifted value (already in place from step 1)
        }
    }

    // ----------------------------------------------------------------------
    // 4. ALWAYS compute median (no border condition!)
    // ----------------------------------------------------------------------
    p_out = p_in;

    uint8_t flat_window[NUM_ELEMENTS];
    #pragma HLS ARRAY_PARTITION variable=flat_window complete dim=0

    int flat_idx = 0;
    for(int i = 0; i < K_SIZE; i++) {
        #pragma HLS UNROLL
        for(int j = 0; j < K_SIZE; j++) {
            #pragma HLS UNROLL
            flat_window[flat_idx++] = window_buffer[i][j];
        }
    }

    uint8_t median_val;
    hls_oddeven_sort_median(flat_window, median_val);

    p_out.data = r2rgba(median_val) | g2rgba(median_val) | b2rgba(median_val) |
                 (p_in.data & 0xFF000000);

    dst << p_out;

    if (p_in.last) {
        x = 0;
        y++;
        cnt++;
        if (cnt >= (K_SIZE - 1)) cnt = 0;
    } else {
        x++;
    }
}

// Optional standalone stream wrapper for testing this stage only
void median_blur_stream(pixel_stream &src, pixel_stream &dst, int frame)
{
    (void)frame;
    median_blur(src, dst);
}