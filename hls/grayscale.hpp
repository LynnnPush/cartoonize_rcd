#ifndef GRAYSCALE_HPP
#define GRAYSCALE_HPP

#include <stdint.h>

#include "pixel_types.hpp"

// Macros for Color Extraction
#define rgba2r(v) ((v) & 0xFF)
#define rgba2g(v) (((v) & 0xFF00) >> 8)
#define rgba2b(v) (((v) & 0xFF0000) >> 16)
#define rgba2a(v) (((v) & 0xFF000000) >> 24)

#define r2rgba(v) ((v) & 0xFF)
#define g2rgba(v) (((v) & 0xFF) << 8)
#define b2rgba(v) (((v) & 0xFF) << 16)
#define a2rgba(v) (((v) & 0xFF) << 24)

// Function Prototype
void grayscale(pixel_stream &src, pixel_stream &dst);

#endif
