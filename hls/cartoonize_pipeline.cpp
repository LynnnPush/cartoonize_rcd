#include "cartoonize_pipeline.h"

// Use existing stage implementations from their respective compilation units.
void median_blur_adaptive_threshold(pixel_stream &src, pixel_stream &dst) {
    #pragma HLS INTERFACE axis port=src
    #pragma HLS INTERFACE axis port=dst
    #pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS DATAFLOW

    pixel_stream blurred_stream("blurred_stream");
    #pragma HLS STREAM variable=blurred_stream depth=64

    median_blur(src, blurred_stream);
    adaptive_threshold(blurred_stream, dst);
}

// Stream() function for streamulator testbench
void stream(pixel_stream &src, pixel_stream &dst, int frame) {
    (void)frame;
    median_blur_adaptive_threshold(src, dst);
}
