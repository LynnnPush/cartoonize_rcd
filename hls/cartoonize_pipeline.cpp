#include "cartoonize_pipeline.h"

// ----------------------------------------------------------------------
// Helper: duplicate incoming stream to two outputs (one pixel per call)
// ----------------------------------------------------------------------
static void duplicate_stream(pixel_stream &src, pixel_stream &dst_a, pixel_stream &dst_b) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    pixel_data p;
    src >> p;
    dst_a << p;
    dst_b << p;
}

// ----------------------------------------------------------------------
// Helper: simple mask-and operation (color & edge mask)
// ----------------------------------------------------------------------
static void bitwise_and_mask(pixel_stream &color, pixel_stream &mask, pixel_stream &dst) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    pixel_data p_color, p_mask, p_out;
    color >> p_color;
    mask >> p_mask;

    // mask uses grayscale replicated to RGB; use LSB byte
    uint8_t mask_val = (uint8_t)(p_mask.data & 0xFF);

    p_out = p_color; // copy metadata (user/last)
    if (mask_val > 0) {
        p_out.data = p_color.data;
    } else {
        p_out.data = 0;
    }

    dst << p_out;
}


// Full cartoonize pipeline: bilateral filter + adaptive threshold + mask composite
void cartoonize_pipeline(pixel_stream &src, pixel_stream &dst) {
    #pragma HLS INTERFACE axis port=src
    #pragma HLS INTERFACE axis port=dst
    #pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS DATAFLOW

    pixel_stream raw_to_bilateral("raw_to_bilateral");
    pixel_stream raw_to_gray("raw_to_gray");
    pixel_stream color_stream("color_stream");
    pixel_stream gray_stream("gray_stream");
    pixel_stream median_stream("median_stream");
    pixel_stream mask_stream("mask_stream");

    #pragma HLS STREAM variable=raw_to_bilateral depth=64
    #pragma HLS STREAM variable=raw_to_gray depth=64
    #pragma HLS STREAM variable=color_stream depth=64
    #pragma HLS STREAM variable=gray_stream depth=64
    #pragma HLS STREAM variable=median_stream depth=64
    #pragma HLS STREAM variable=mask_stream depth=64

    duplicate_stream(src, raw_to_bilateral, raw_to_gray);
    bilateral_filter(raw_to_bilateral, color_stream);
    grayscale(raw_to_gray, gray_stream);
    median_blur(gray_stream, median_stream);
    adaptive_threshold(median_stream, mask_stream);
    bitwise_and_mask(color_stream, mask_stream, dst);
}

// Stream() function for streamulator testbench
void stream(pixel_stream &src, pixel_stream &dst, int frame) {
    (void)frame;
    cartoonize_pipeline(src, dst);
}
