#ifndef AXIPNG_H
#define AXIPNG_H

#include <stdio.h>


#include "spng.h"

#include "../../pixel_types.hpp"


// Read/write PNG images using spng
int pngread (const std::string &filename, unsigned char **idata, struct spng_ihdr *ihdr);
int pngwrite (const std::string &filename, const unsigned char *idata, struct spng_ihdr *ihdr);

// Convert between AXI stream and image data
int img2axi (const unsigned char *idata, const struct spng_ihdr &ihdr, pixel_stream &stream);
//int axi2img (unsigned char *idata, const struct spng_ihdr &ihdr, pixel_stream &stream);


#endif // AXIPNG_H
