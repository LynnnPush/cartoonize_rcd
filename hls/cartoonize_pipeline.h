#ifndef CARTOONIZE_PIPELINE_HPP
#define CARTOONIZE_PIPELINE_HPP

#include <stdint.h>
#include <ap_int.h>

#include "pixel_types.hpp"

// Forward declarations of stage functions
void grayscale(pixel_stream &src, pixel_stream &dst);
void bilateral_filter(pixel_stream &src, pixel_stream &dst);
void median_blur(pixel_stream &src, pixel_stream &dst);
void adaptive_threshold(pixel_stream &src, pixel_stream &dst);
// Debug helper: simple pass-through stage
void pixel_passthrough(pixel_stream &src, pixel_stream &dst);

// Combined sub-pipeline: median blur -> adaptive threshold (edge mask)
void median_blur_adaptive_threshold(pixel_stream &src, pixel_stream &dst);

// Full cartoonize pipeline: bilateral color smoothing + edge mask + composite
void cartoonize_pipeline(axis_stream &src, axis_stream &dst, uint32_t mode);

// Top-level entry for streamulator/testbench
void stream(axis_stream &src, axis_stream &dst, int frame);

// Mode bit definitions
enum FilterMode : uint32_t {
    MODE_NONE      = 0x00, // no effect

    MODE_BILATERAL = 0x01, // bit 0
    MODE_GRAY      = 0x02, // bit 1
    MODE_MEDIAN    = 0x04, // bit 2
    MODE_THRESHOLD = 0x08, // bit 3
    MODE_MASK      = 0x10, // bit 4

    MODE_ALL       = 0x1F  // all effects enabled
};

#endif
