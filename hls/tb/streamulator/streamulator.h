// Streamulator test platform

#ifndef INC_H
#define INC_H


#include <stdint.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#include "axipng.h"

// Image dimensions
#define WIDTH 1280
#define HEIGHT 720

// Image paths
#define INPUT_DIR  "D:/PracticeProject/TUD_RClab/cartoonize_rcd/hls/tb/inframe/"
#define OUTPUT_DIR "D:/PracticeProject/TUD_RClab/cartoonize_rcd/hls/tb/outframe/"

#define FRAMES_LIST "D:/PracticeProject/TUD_RClab/cartoonize_rcd/hls/tb/inframe/frames.txt"

//#define RAW_FILENAME "../../../../outframes/raw.png"


// Stream processing function
void stream (pixel_stream &src, pixel_stream &dst, int frame);


#endif // INC_H
