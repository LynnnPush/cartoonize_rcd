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

// Full cartoonize pipeline with mode selection
// NOTE: All processing stages run in parallel; mode selects which output to use
void cartoonize_pipeline_sel(axis_stream &src, axis_stream &dst, uint32_t mode);

// Top-level entry for streamulator/testbench
void stream(pixel_stream &src, pixel_stream &dst, int frame);

// Mode definitions
// These modes select which processing result to output
// All stages run regardless of mode selection (for proper dataflow)
enum FilterMode : uint32_t {
    MODE_NONE              = 0, // Raw passthrough (no filters)
    MODE_BILATERAL         = 1, // Bilateral filter only (smoothed color)
    MODE_GRAYSCALE         = 2, // Grayscale conversion only
    MODE_MEDIAN            = 3, // Grayscale + median blur
    MODE_ADAPTIVE_THRESHOLD = 4, // Grayscale + median + threshold (edge mask)
    MODE_FULL_CARTOON      = 5  // Bilateral color with edge mask overlay
};

#endif