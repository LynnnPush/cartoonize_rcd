#ifndef ADAPTIVE_THRESHOLD_HPP
#define ADAPTIVE_THRESHOLD_HPP

#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>

// --------------------------------------------------------------------------
// Typedefs (Matching canny.hpp structure)
// --------------------------------------------------------------------------
typedef ap_axiu<32, 1, 1, 1> pixel_data;
typedef hls::stream<pixel_data> pixel_stream;

// Macros for extracting RGB (We only need Red/Gray for this module)
#define rgba2r(v) ((v)&0xFF)
#define rgba2g(v) (((v)&0xFF00) >> 8)
#define rgba2b(v) (((v)&0xFF0000) >> 16)

#define r2rgba(v) ((v)&0xFF)
#define g2rgba(v) (((v)&0xFF) << 8)
#define b2rgba(v) (((v)&0xFF) << 16)

// --------------------------------------------------------------------------
// Constants
// --------------------------------------------------------------------------
#define WIDTH 1280
#define HEIGHT 720

// Adaptive Threshold Parameters
#define K_SIZE 7
#define K_PAD (K_SIZE / 2)
#define K_AREA (K_SIZE * K_SIZE)
#define C_CONST 7
#define MAX_VAL 255

// --------------------------------------------------------------------------
// Function Prototype
// --------------------------------------------------------------------------
/**
 * Adaptive Thresholding (Optimized Sliding Sum)
 * Input: Grayscale Stream (e.g., after Median Blur)
 * Output: Binary Edge Stream
 */
void adaptive_threshold(pixel_stream &src, pixel_stream &dst);

#endif