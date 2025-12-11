#ifndef KERNELS_HPP
#define KERNELS_HPP

#include <cstddef>

// Declare sizes (or make them constexpr)
constexpr std::size_t D = 5;
constexpr std::size_t COLOR_LUT_SIZE = 256;

// Declare the lookup tables (defined in .cpp)
extern const float SPATIAL_KERNEL[D][D];
extern const float COLOR_LUT[COLOR_LUT_SIZE];

#endif
