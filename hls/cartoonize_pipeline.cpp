#include "cartoonize_pipeline.h"

static void duplicate_stream(pixel_stream &src, pixel_stream &dst_a, pixel_stream &dst_b) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    pixel_data p;
    src >> p;
    dst_a << p;
    dst_b << p;
}

static void axis_to_pixel(axis_stream &s_axis, pixel_stream &dst) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    axis_pixel in_ax;
    s_axis >> in_ax;

    pixel_data p;
    p.data = in_ax.data;
    p.keep = in_ax.keep;
    p.strb = in_ax.strb;
    p.user = in_ax.user;
    p.last = in_ax.last;
    p.id   = in_ax.id;
    p.dest = in_ax.dest;

    dst << p;
}

static void bitwise_and_mask(pixel_stream &color, pixel_stream &mask, pixel_stream &dst) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    pixel_data p_color, p_mask, p_out;
    color >> p_color;
    mask >> p_mask;

    uint8_t mask_val = (uint8_t)(p_mask.data & 0xFF);

    p_out = p_color;
    if (mask_val > 0) {
        p_out.data = p_color.data;
    } else {
        p_out.data = (p_color.data & 0xFF000000);
    }

    dst << p_out;
}

static void pixel_to_axis(pixel_stream &src, axis_stream &d_axis) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    pixel_data p;
    src >> p;

    axis_pixel out_ax;
    out_ax.data = p.data;
    out_ax.keep = p.keep;
    out_ax.strb = p.strb;
    out_ax.user = p.user;
    out_ax.last = p.last;
    out_ax.id   = p.id;
    out_ax.dest = p.dest;

    d_axis << out_ax;
}

void pixel_passthrough(pixel_stream &src, pixel_stream &dst) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    pixel_data p;
    src >> p;
    dst << p;
}

void cartoonize_pipeline_v2(axis_stream &src, axis_stream &dst,
                            uint32_t mode) {
    #pragma HLS INTERFACE axis port=src
    #pragma HLS INTERFACE axis port=dst
    #pragma HLS INTERFACE s_axilite port=mode
    #pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS DATAFLOW disable_start_propagation

    bool en_bilateral = false;
    bool en_gray      = false;
    bool en_median    = false;
    bool en_thresh    = false;
    bool en_mask      = false;
    bool out_gray     = false;

    switch ((FilterMode)mode) {

    case MODE_NONE:
        break;

    case MODE_BILATERAL:
        en_bilateral = true;
        break;

    case MODE_GRAYSCALE:
        en_gray  = true;
        out_gray = true;
        break;

    case MODE_MEDIAN:
        en_gray   = true;
        en_median = true;
        out_gray  = true;
        break;

    case MODE_GRAY_MEDIAN:
        en_gray   = true;
        en_median = true;
        out_gray  = true;
        break;

    case MODE_FULL_CARTOON:
        en_bilateral = true;
        en_gray      = true;
        en_median    = true;
        en_thresh    = true;
        en_mask      = true;
        break;

    default:
        break;
    }

    pixel_stream in_pix("in_pix");
    pixel_stream out_pix("out_pix");
    #pragma HLS STREAM variable=in_pix depth=64
    #pragma HLS STREAM variable=out_pix depth=64

    pixel_stream raw_to_bilateral("raw_to_bilateral");
    pixel_stream raw_to_gray("raw_to_gray");
    pixel_stream color_stream("color_stream");
    pixel_stream gray_stream("gray_stream");
    pixel_stream median_stream("median_stream");
    pixel_stream mask_stream("mask_stream");

    #pragma HLS STREAM variable=raw_to_bilateral depth=64
    #pragma HLS STREAM variable=raw_to_gray depth=64
    #pragma HLS STREAM variable=color_stream depth=64
    #pragma HLS STREAM variable=gray_stream depth=64
    #pragma HLS STREAM variable=median_stream depth=64
    #pragma HLS STREAM variable=mask_stream depth=64

    axis_to_pixel(src, in_pix);

    duplicate_stream(in_pix, raw_to_bilateral, raw_to_gray);

    if (en_bilateral)
        bilateral_filter(raw_to_bilateral, color_stream);
    else
        pixel_passthrough(raw_to_bilateral, color_stream);

    if (en_gray)
        grayscale(raw_to_gray, gray_stream);
    else
        pixel_passthrough(raw_to_gray, gray_stream);

    if (en_median)
        median_blur(gray_stream, median_stream);
    else
        pixel_passthrough(gray_stream, median_stream);

    if (en_thresh)
        adaptive_threshold(median_stream, mask_stream);
    else
        pixel_passthrough(median_stream, mask_stream);

    if (en_mask)
        bitwise_and_mask(color_stream, mask_stream, out_pix);
    else if (out_gray)
        pixel_passthrough(median_stream, out_pix);
    else
        pixel_passthrough(color_stream, out_pix);

    pixel_to_axis(out_pix, dst);
}

void stream(pixel_stream &src, pixel_stream &dst, int frame) {
    (void)frame;
    axis_stream axis_src("axis_src");
    axis_stream axis_dst("axis_dst");

    uint32_t mode = MODE_FULL_CARTOON;

    pixel_to_axis(src, axis_src);
    cartoonize_pipeline_v2(axis_src, axis_dst, mode);
    axis_to_pixel(axis_dst, dst);
}
