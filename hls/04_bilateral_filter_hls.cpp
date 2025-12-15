#include "adaptive_threshold.h"
//#include "04_bilateral_filter_gaussian_data.hpp"
#include "04_bilateral_filter_kernels_fixed.hpp"
#include <ap_int.h>
#include "04_bilateral_reciprocal_lut.hpp"


#include <ap_fixed.h>

// Accumulator types
typedef ap_ufixed<24,8>  wsum_t;   // up to ~121, enough headroom
typedef ap_ufixed<32,16> vsum_t;   // up to ~32000


// =========================================================================
// CONFIGURATION CONSTANTS
// WIDTH and K_SIZE are used from the included headers
// =========================================================================
#define BF_PAD (D / 2) // D comes from 04_bilateral_filter_gaussian_data.hpp

// Helper struct for easier RGB access (B, G, R order matches your initial code)
struct rgb_pixel {
    uint8_t val[3]; // 0: Blue, 1: Green, 2: Red
};

// Helper to unpack 32-bit AXI stream data to RGB
inline rgb_pixel unpack_rgb(uint32_t data) {
    #pragma HLS INLINE
    rgb_pixel p;
    // We use the macros from adaptive_threshold.h, but re-implemented for a struct
    p.val[0] = rgba2b(data);       // Blue (assuming RGBA2R is actually for the 0-7 bits)
    p.val[1] = rgba2g(data);       // Green
    p.val[2] = rgba2r(data);       // Red
    return p;
}

// Helper to repack RGB to 32-bit AXI stream data
inline uint32_t pack_rgb(rgb_pixel p) {
    #pragma HLS INLINE
    // We use the macros from adaptive_threshold.h
    return r2rgba(p.val[2]) | g2rgba(p.val[1]) | b2rgba(p.val[0]);
}

void bilateral_filter(pixel_stream &src, pixel_stream &dst)
{
    // ----------------------------------------------------------------------
    // 0. INTERFACES & PRAGMAS
    // ----------------------------------------------------------------------
    #pragma HLS INTERFACE axis port=src
    #pragma HLS INTERFACE axis port=dst
    #pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS PIPELINE II=1 // Target Initiation Interval of 1

    // ----------------------------------------------------------------------
    // 1. STATE & BUFFERS
    // ----------------------------------------------------------------------
    static uint16_t x = 0;
    static uint16_t y = 0;
    static uint8_t line_idx = 0; // Cyclic buffer index (0 to D-2)

    // LINE BUFFER: Stores D-1 (4) rows of RGB pixels
    static rgb_pixel line_buffer[D - 1][WIDTH];
    #pragma HLS ARRAY_PARTITION variable=line_buffer dim=1 complete // Partition row index (D-1) for parallel read
    #pragma HLS DEPENDENCE variable=line_buffer inter false // Optimize access pattern

    // WINDOW BUFFER: The active DxD (5x5) pixel kernel
    static rgb_pixel window_buffer[D][D];
    #pragma HLS ARRAY_PARTITION variable=window_buffer complete dim=0 // Partition all dimensions for max parallelism

    // ----------------------------------------------------------------------
    // 2. INPUT READ & SYNC
    // ----------------------------------------------------------------------
    
    pixel_data p_in;
    src >> p_in;

    if (p_in.user) {
        x = 0; y = 0; line_idx = 0;
    }

    rgb_pixel new_pixel = unpack_rgb(p_in.data);

    // ----------------------------------------------------------------------
    // 3. BUFFER MANAGEMENT (Line Buffer & Window Shift)
    // ----------------------------------------------------------------------

    // Shift Window Left (Move all pixels one column to the left)
    for(int i=0; i < D; i++) {
        #pragma HLS UNROLL
        for(int j=0; j < D - 1; j++) {
            #pragma HLS UNROLL
            window_buffer[i][j] = window_buffer[i][j+1]; 
        }
    }

    // Fill Rightmost Column (Update)
    if (x < WIDTH) {
        // 1. Fill top D-1 pixels from line buffer (circular logic)
        for (int i = 0; i < D - 1; i++) {
            #pragma HLS UNROLL
            // Calculate cyclic index to read chronologically correct rows
            uint8_t row_offset = (line_idx + i) % (D - 1); 
            window_buffer[i][D - 1] = line_buffer[row_offset][x];
        }
        
        // 2. Bottom pixel is the current incoming pixel
        window_buffer[D - 1][D - 1] = new_pixel;

        // 3. Update Line Buffer with new pixel (overwriting oldest line)
        line_buffer[line_idx][x] = new_pixel;
    }

    // ----------------------------------------------------------------------
    // 4. BILATERAL FILTER CORE LOGIC
    // ----------------------------------------------------------------------
    
    pixel_data p_out = p_in; 
    rgb_pixel result_pixel = {0, 0, 0};

    // Output valid only after window is filled (D-1 in y and D-1 in x)
    if (y >= D - 1 && x >= D - 1) {
        
        rgb_pixel center_px = window_buffer[BF_PAD][BF_PAD];

        // Process R, G, B channels independently
        for (int c = 0; c < 3; c++) {
            #pragma HLS UNROLL // Process all 3 channels in parallel
            
            wsum_t w_sum = 0;
            vsum_t val_sum = 0;

            uint8_t center_val = center_px.val[c];

            // Convolve over the 5x5 window (Fully Unrolled 25 taps in parallel)
            for (int i = 0; i < D; i++) {
                #pragma HLS UNROLL
                for (int j = 0; j < D; j++) {
                    #pragma HLS UNROLL
                    
                    uint8_t neighbor_val = window_buffer[i][j].val[c];
                    
                    // 1. Calculate Absolute Intensity Difference (0 to 255)
                    int diff_int = (int)neighbor_val - (int)center_val;
                    // Optimized absolute value for HLS
                    int diff_abs = (diff_int ^ (diff_int >> 31)) - (diff_int >> 31);
                    
                    // 2. Look up Weights (from on-chip ROMs)
                    weight_t w_s = SPATIAL_KERNEL_FX[i][j];
                    weight_t w_c = COLOR_LUT_FX[diff_abs];

                    weight_t w = w_s * w_c;

                    // 3. Accumulate
                    w_sum   += w;
                    val_sum += w * neighbor_val;
                }
            }

            // 4. Normalize (reciprocal LUT + multiply)
            // Quantize w_sum to LUT index
            ap_uint<8> recip_idx = w_sum.to_uint();
            if (recip_idx == 0) recip_idx = 1;
            if (recip_idx > 255) recip_idx = 255;
            recip_idx -= 1;


            // Lookup reciprocal
            recip_t inv_w = RECIP_LUT[recip_idx];

            // Normalize
            ap_ufixed<32,16> norm = val_sum * inv_w;

            // Clamp and assign
            if (norm > 255)
                result_pixel.val[c] = 255;
            else
                result_pixel.val[c] = (uint8_t)norm;


        }
    }

    // ----------------------------------------------------------------------
    // 5. OUTPUT & COORDINATE UPDATES
    // ----------------------------------------------------------------------

    p_out.data = pack_rgb(result_pixel);
    
    dst << p_out; // Write to Output stream

    // Update Counters
    if (p_in.last) {
        x = 0;
        y++;
        // Cycle the line buffer index
        line_idx++;
        if (line_idx >= (D - 1)) line_idx = 0;
    } else {
        x++;
    }
}

// Optional standalone stream wrapper for testing this stage only
void bilateral_filter_stream(pixel_stream &src, pixel_stream &dst, int frame)
{
    (void)frame;
    bilateral_filter(src, dst);
}