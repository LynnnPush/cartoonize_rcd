#ifndef CARTOONIZE_PIPELINE_HPP
#define CARTOONIZE_PIPELINE_HPP

#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>

// Shared stream types
typedef ap_axiu<32, 1, 1, 1> pixel_data;
typedef hls::stream<pixel_data> pixel_stream;

// Forward declarations of stage functions
void median_blur(pixel_stream &src, pixel_stream &dst);
void adaptive_threshold(pixel_stream &src, pixel_stream &dst);

// Combined pipeline: median blur -> adaptive threshold
void median_blur_adaptive_threshold(pixel_stream &src, pixel_stream &dst);

// Top-level entry for streamulator/testbench
void stream(pixel_stream &src, pixel_stream &dst, int frame);

#endif
