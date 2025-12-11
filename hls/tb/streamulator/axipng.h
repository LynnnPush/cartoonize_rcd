#ifndef AXIPNG_H
#define AXIPNG_H

#include <stdio.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#include "spng.h"


// Pixel and stream types
typedef ap_axiu<32,1,1,1> pixel_data;
typedef hls::stream<pixel_data> pixel_stream;


// Read/write PNG images using spng
int pngread (const std::string &filename, unsigned char **idata, struct spng_ihdr *ihdr);
int pngwrite (const std::string &filename, const unsigned char *idata, struct spng_ihdr *ihdr);

// Convert between AXI stream and image data
int img2axi (const unsigned char *idata, const struct spng_ihdr &ihdr, pixel_stream &stream);
//int axi2img (unsigned char *idata, const struct spng_ihdr &ihdr, pixel_stream &stream);


#endif // AXIPNG_H
