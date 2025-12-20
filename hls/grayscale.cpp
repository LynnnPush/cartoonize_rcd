#include "grayscale.hpp"

void grayscale(pixel_stream &src, pixel_stream &dst)
{
    #pragma HLS PIPELINE II=1

    static uint16_t x = 0;
    static uint16_t y = 0;

    // 1. Input Read
    pixel_data p_in;
    src >> p_in; 

    // Handle Start of Frame (TUSER) signal
    if (p_in.user) {
        x = 0;
        y = 0;
    }

    // 2. Computation (RGB to Gray)
    
    // Extract RGB channels
    uint8_t r = rgba2r(p_in.data);
    uint8_t g = rgba2g(p_in.data);
    uint8_t b = rgba2b(p_in.data);

    // Gray = (R * 76 + G * 150 + B * 29) >> 8
    uint16_t gray_val = (r * 76 + g * 150 + b * 29) >> 8;

    // Clamp value to 255 (Safety check)
    uint8_t gray_clamped = (gray_val > 255) ? 255 : (uint8_t)gray_val;

    // 3. Output Write
    pixel_data p_out = p_in; // Copy metadata (user/last)

    // Replicate the grayscale value across R, G, and B channels.
    p_out.data = r2rgba(gray_clamped) |
                 g2rgba(gray_clamped) |
                 b2rgba(gray_clamped) |
                 (p_in.data & 0xFF000000); // preserve MSB/alpha

    // Write to output stream
    dst << p_out;

    // Update internal counters
    if (p_in.last) {
        x = 0;
        y++;
    } else {
        x++;
    }
}

// Optional standalone stream wrapper for testing this stage only
void grayscale_stream(pixel_stream &src, pixel_stream &dst, int frame)
{
    (void)frame;
    grayscale(src, dst);
}
