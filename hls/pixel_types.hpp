#ifndef PIXEL_TYPES_HPP
#define PIXEL_TYPES_HPP

#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

// Simple AXI-stream compatible pixel packet for internal streams
struct pixel_data {
    ap_uint<32> data;
    ap_uint<1>  user;
    ap_uint<1>  last;
    ap_uint<4>  keep;
    ap_uint<4>  strb;
    ap_uint<1>  id;
    ap_uint<1>  dest;
};

typedef hls::stream<pixel_data> pixel_stream;

// Use only for top-level ports
typedef ap_axiu<32,1,1,1> axis_pixel;
typedef hls::stream<axis_pixel> axis_stream;

#endif // PIXEL_TYPES_HPP
