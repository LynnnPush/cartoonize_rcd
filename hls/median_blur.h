#ifndef MEDIAN_BLUR_HPP
#define MEDIAN_BLUR_HPP

#include <ap_int.h>     
#include <hls_stream.h> 
#include <ap_axi_sdata.h>
#include <stdint.h>

// Constants
#define K_SIZE 5
#define WIDTH 1280
#define HEIGHT 720
#define NUM_ELEMENTS (K_SIZE * K_SIZE)

// Data Types
typedef ap_axiu<32,1,1,1> pixel_data;
typedef hls::stream<pixel_data> pixel_stream;

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