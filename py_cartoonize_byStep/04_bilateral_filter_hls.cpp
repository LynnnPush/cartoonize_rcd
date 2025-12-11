#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cmath>


#include "04_bilateral_filter_gaussian_data.hpp"

// ============================================================
// Image structure (BGR interleaved, no external libraries)
// ============================================================
struct Image {
    int width;
    int height;
    std::vector<uint8_t> data;  // B,G,R,B,G,R,...

    Image() : width(0), height(0) {}
    Image(int w, int h) : width(w), height(h), data(w*h*3, 0) {}

    inline uint8_t& at(int y, int x, int c) {
        return data[(y * width + x) * 3 + c];
    }
    inline const uint8_t& at(int y, int x, int c) const {
        return data[(y * width + x) * 3 + c];
    }
};

// ============================================================
// BMP Loader (24-bit only)
// ============================================================
Image loadBMP(const std::string& filename) {
    std::ifstream f(filename, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open BMP file: " + filename);

    uint16_t bfType;
    f.read((char*)&bfType, 2);
    if (bfType != 0x4D42)
        throw std::runtime_error("Not a BMP file");

    f.seekg(18);
    int width, height;
    f.read((char*)&width, 4);
    f.read((char*)&height, 4);

    uint16_t planes, bitCount;
    f.read((char*)&planes, 2);
    f.read((char*)&bitCount, 2);

    if (bitCount != 24)
        throw std::runtime_error("Only 24-bit BMP supported");

    f.seekg(54);

    Image img(width, height);
    const int rowPadded = (width * 3 + 3) & ~3;
    std::vector<uint8_t> row(rowPadded);

    for (int y = height - 1; y >= 0; --y) {
        f.read((char*)row.data(), rowPadded);
        for (int x = 0; x < width; ++x) {
            img.at(y, x, 0) = row[x*3 + 0];
            img.at(y, x, 1) = row[x*3 + 1];
            img.at(y, x, 2) = row[x*3 + 2];
        }
    }

    return img;
}

// ============================================================
// BMP Writer
// ============================================================
void saveBMP(const std::string& filename, const Image& img) {
    std::ofstream f(filename, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot write BMP file: " + filename);

    const int rowPadded = (img.width * 3 + 3) & ~3;
    const int fileSize  = 54 + rowPadded * img.height;

    uint8_t header[54] = {
        'B','M',
        0,0,0,0, 0,0,0,0,
        54,0,0,0, 40,0,0,0
    };

    // File size
    header[2] = fileSize;
    header[3] = fileSize >> 8;
    header[4] = fileSize >> 16;
    header[5] = fileSize >> 24;

    // Width
    header[18] = img.width;
    header[19] = img.width >> 8;
    header[20] = img.width >> 16;
    header[21] = img.width >> 24;

    // Height
    header[22] = img.height;
    header[23] = img.height >> 8;
    header[24] = img.height >> 16;
    header[25] = img.height >> 24;

    header[26] = 1;  // planes
    header[28] = 24; // 24-bit BMP

    f.write((char*)header, 54);

    std::vector<uint8_t> row(rowPadded);

    for (int y = img.height - 1; y >= 0; --y) {
        for (int x = 0; x < img.width; ++x) {
            row[x*3 + 0] = img.at(y, x, 0);
            row[x*3 + 1] = img.at(y, x, 1);
            row[x*3 + 2] = img.at(y, x, 2);
        }
        f.write((char*)row.data(), rowPadded);
    }
}

// ============================================================
// Bilateral Filter (your HLS model translated into C++)
// ============================================================
Image bilateral_filter_hls_model_cpp(const Image& img, int d)
{
    const int height = img.height;
    const int width  = img.width;

    Image out(width, height);

    const int center = d / 2;
    const int ch = 3;

    std::vector<uint8_t> line_buffer((d - 1) * width * ch, 0);
    auto LB = [&](int row, int x, int c)->uint8_t& {
        return line_buffer[(row * width + x) * ch + c];
    };

    std::vector<uint8_t> window(d * d * ch, 0);
    auto W = [&](int i, int j, int c)->uint8_t& {
        return window[(i * d + j) * ch + c];
    };

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {

            uint8_t new_px[3] = {
                img.at(y, x, 0),
                img.at(y, x, 1),
                img.at(y, x, 2)
            };

            // 1) Shift window left
            for (int i = 0; i < d; ++i)
                for (int j = 0; j < d - 1; ++j)
                    for (int c = 0; c < ch; ++c)
                        W(i, j, c) = W(i, j+1, c);

            // 2) Fill right column
            if (y >= 1) {
                for (int i = 0; i < d - 1; ++i)
                    for (int c = 0; c < ch; ++c)
                        W(i, d-1, c) = LB(i, x, c);
            } else {
                for (int i = 0; i < d - 1; ++i)
                    for (int c = 0; c < ch; ++c)
                        W(i, d-1, c) = 0;
            }

            for (int c = 0; c < ch; ++c)
                W(d-1, d-1, c) = new_px[c];

            // 3) Update line buffer
            if (d > 1) {
                for (int i = 0; i < d-2; ++i)
                    for (int c = 0; c < ch; ++c)
                        LB(i, x, c) = LB(i+1, x, c);

                for (int c = 0; c < ch; ++c)
                    LB(d-2, x, c) = new_px[c];
            }

            // 4) Produce output?
            if (y >= d-1 && x >= d-1) {

                int out_y = y - center;
                int out_x = x - center;

                for (int c = 0; c < ch; ++c)
                {
                    int center_val = W(center, center, c);
                    double norm = 0.0, wsum = 0.0;

                    for (int i = 0; i < d; ++i)
                        for (int j = 0; j < d; ++j)
                        {
                            int neigh = W(i, j, c);
                            int diff = std::abs(neigh - center_val);

                            float w_color = COLOR_LUT[diff];
                            float w = SPATIAL_KERNEL[i][j] * w_color;

                            wsum += neigh * w;
                            norm += w;
                        }

                    out.at(out_y, out_x, c) =
                        static_cast<uint8_t>(wsum / (norm + 1e-8));
                }
            }
        }
    }

    return out;
}

// ============================================================
// MAIN
// ============================================================
int main() {
    try {
        // Load input image (24-bit BMP only)
        Image img = loadBMP("input/Things-to-do-in-Delft.bmp");

        // Run bilateral filter (your C++ HLS-style model)
        Image filtered = bilateral_filter_hls_model_cpp(img, 5);

        // Save output
        saveBMP("output/step4_bilateral_.bmp", filtered);

        std::cout << "Saved output/step4_bilateral_2.bmp\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}

