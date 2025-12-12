#ifndef MEDIAN_BLUR_HPP
#define MEDIAN_BLUR_HPP

#include <ap_int.h>     
#include <stdint.h>

#include "pixel_types.hpp"

// Constants
#define K_SIZE 5
#define WIDTH 1280
#define HEIGHT 720
#define NUM_ELEMENTS (K_SIZE * K_SIZE)

// Macros for RGB/Grayscale extraction
#define rgba2r(v) ((v)&0xFF)
#define rgba2g(v) (((v)&0xFF00) >> 8)
#define rgba2b(v) (((v)&0xFF0000) >> 16)
#define rgba2a(v) (((v)&0xFF000000) >> 24)

#define r2rgba(v) ((v)&0xFF)
#define g2rgba(v) (((v)&0xFF) << 8)
#define b2rgba(v) (((v)&0xFF) << 16)
#define a2rgba(v) (((v)&0xFF) << 24)

// 3. Function Prototype
void hls_bubble_sort(uint8_t input_arr[NUM_ELEMENTS], uint8_t &median);
void median_blur(pixel_stream &src, pixel_stream &dst);

#endif
