#ifndef PIXEL_TYPES_HPP
#define PIXEL_TYPES_HPP

#include <ap_int.h>
#include <hls_stream.h>

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

#endif // PIXEL_TYPES_HPP
