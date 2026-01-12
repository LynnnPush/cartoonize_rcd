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
void cartoonize_pipeline_sel(axis_stream &src, axis_stream &dst, uint32_t mode);

// Top-level entry for streamulator/testbench
void stream(axis_stream &src, axis_stream &dst, int frame);

// Mode bit definitions
enum FilterMode : uint32_t {
    MODE_NONE              = 0, // no filters
    MODE_BILATERAL         = 1, // bilateral only
    MODE_GRAYSCALE         = 2, // grayscale only
    MODE_MEDIAN            = 3, // median only (on grayscale path)
    MODE_ADAPTIVE_THRESHOLD = 4, // grayscale + median + threshold
    MODE_FULL_CARTOON      = 5  // bilateral + grayscale + median + threshold + mask
};

#endif
