#include "cartoonize_pipeline.h"

// ============================================================================
// SOLUTION: Remove all runtime conditionals from the DATAFLOW region
// 
// The problem was that if/else statements in DATAFLOW regions are not allowed
// (non-canonical statements). HLS merges them into a single process with
// II=6-31 instead of II=1, making the pipeline too slow for real-time video.
//
// The fix: ALWAYS run ALL processing stages, then use a multiplexer at the
// output to select which result to use based on the mode.
// ============================================================================

// ----------------------------------------------------------------------
// Helper: split stream to three outputs (for mode selection flexibility)
// ----------------------------------------------------------------------
static void triplicate_stream(pixel_stream &src, 
                              pixel_stream &dst_a, 
                              pixel_stream &dst_b,
                              pixel_stream &dst_c) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    pixel_data p;
    src >> p;
    dst_a << p;
    dst_b << p;
    dst_c << p;
}

// ----------------------------------------------------------------------
// Helper: duplicate incoming stream to two outputs (one pixel per call)
// ----------------------------------------------------------------------
static void duplicate_stream(pixel_stream &src, pixel_stream &dst_a, pixel_stream &dst_b) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    pixel_data p;
    src >> p;
    dst_a << p;
    dst_b << p;
}

// ----------------------------------------------------------------------
// Helper: AXI stream -> internal pixel stream (one pixel per call)
// ----------------------------------------------------------------------
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

// ----------------------------------------------------------------------
// Helper: internal pixel stream -> AXI stream (one pixel per call)
// ----------------------------------------------------------------------
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

// ----------------------------------------------------------------------
// Public helper: simple pass-through stage (one pixel per call)
// ----------------------------------------------------------------------
void pixel_passthrough(pixel_stream &src, pixel_stream &dst) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    pixel_data p;
    src >> p;
    dst << p;
}

// ============================================================================
// OUTPUT SELECTOR: All inputs ALWAYS read (canonical dataflow requirement)
// 
// This function reads from ALL input streams unconditionally, ensuring
// proper dataflow behavior, then uses combinational MUX logic to select output.
//
// IMPORTANT: Each stream must be read exactly once per pixel!
// ============================================================================
static void output_selector_6way(
    pixel_stream &raw_passthrough,  // Mode 0: raw input (no processing)
    pixel_stream &bilateral_out,    // Mode 1: bilateral filtered color
    pixel_stream &gray_out,         // Mode 2: grayscale only
    pixel_stream &median_out,       // Mode 3: median filtered grayscale
    pixel_stream &thresh_out,       // Mode 4: edge mask
    pixel_stream &cartoon_color,    // Mode 5: bilateral color for cartoon composite
    pixel_stream &dst,              // Final output
    ap_uint<3> mode                 // Mode selection
) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1

    // CRITICAL: ALWAYS read from ALL input streams unconditionally
    pixel_data p_raw, p_bilateral, p_gray, p_median, p_thresh, p_cartoon;
    
    raw_passthrough >> p_raw;
    bilateral_out >> p_bilateral;
    gray_out >> p_gray;
    median_out >> p_median;
    thresh_out >> p_thresh;
    cartoon_color >> p_cartoon;

    // Output selection using MUX logic
    pixel_data p_out;
    
    switch (mode) {
    case 0:  // MODE_NONE - raw passthrough
        p_out = p_raw;
        break;
        
    case 1:  // MODE_BILATERAL - bilateral filtered color
        p_out = p_bilateral;
        break;
        
    case 2:  // MODE_GRAYSCALE - grayscale only
        p_out = p_gray;
        break;
        
    case 3:  // MODE_MEDIAN - median blur on grayscale
        p_out = p_median;
        break;
        
    case 4:  // MODE_ADAPTIVE_THRESHOLD - edge detection
        p_out = p_thresh;
        break;
        
    case 5:  // MODE_FULL_CARTOON - bilateral with edge mask
    default:
        {
            // Apply edge mask to bilateral (cartoon) output
            uint8_t mask_val = (uint8_t)(p_thresh.data & 0xFF);
            p_out = p_cartoon;  // Copy metadata from cartoon color stream
            if (mask_val == 0) {
                // Edge pixel - set to black (keep alpha)
                p_out.data = (p_cartoon.data & 0xFF000000);
            }
            // else: keep the bilateral color
        }
        break;
    }

    dst << p_out;
}

// ============================================================================
// FIXED CARTOONIZE PIPELINE
//
// Key changes from original:
// 1. ALL processing stages run unconditionally in parallel
// 2. Multiple stream copies created using duplicate/triplicate functions
// 3. Output selector reads ALL streams and uses MUX to choose output
// 4. NO if/else around function calls - all functions are always called
// 
// Stream topology:
//                    ┌─► raw_passthrough ──────────────────────────────┐
//                    │                                                  │
// in_pix ─► split3 ──┼─► bilateral ─► dup ─┬─► bilateral_out ──────────┼─► output_selector ─► out_pix
//                    │                     └─► cartoon_color ──────────┤
//                    │                                                  │
//                    └─► grayscale ─► dup ─┬─► gray_out ───────────────┤
//                                          └─► median ─► dup ─┬─► median_out ──────┤
//                                                             └─► adaptive_thresh ─┘
// ============================================================================
void cartoonize_pipeline_sel(axis_stream &src, axis_stream &dst, uint32_t mode) {
    #pragma HLS INTERFACE axis port=src
    #pragma HLS INTERFACE axis port=dst
    #pragma HLS INTERFACE s_axilite port=mode
    #pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS DATAFLOW disable_start_propagation

    // Convert mode to smaller type for stable switch synthesis
    ap_uint<3> mode_sel = (ap_uint<3>)(mode & 0x7);

    // ========================================================================
    // Stream declarations with sufficient depth for pipeline latency buffering
    // ========================================================================
    
    // Input/output streams
    pixel_stream in_pix("in_pix");
    pixel_stream out_pix("out_pix");
    #pragma HLS STREAM variable=in_pix depth=64
    #pragma HLS STREAM variable=out_pix depth=64

    // Initial 3-way split: raw passthrough, bilateral path, grayscale path
    pixel_stream raw_passthrough("raw_passthrough");
    pixel_stream to_bilateral("to_bilateral");
    pixel_stream to_grayscale("to_grayscale");
    #pragma HLS STREAM variable=raw_passthrough depth=64
    #pragma HLS STREAM variable=to_bilateral depth=64
    #pragma HLS STREAM variable=to_grayscale depth=64

    // Bilateral filter outputs (need two copies for different output modes)
    pixel_stream bilateral_internal("bilateral_internal");
    pixel_stream bilateral_out("bilateral_out");
    pixel_stream cartoon_color("cartoon_color");
    #pragma HLS STREAM variable=bilateral_internal depth=64
    #pragma HLS STREAM variable=bilateral_out depth=64
    #pragma HLS STREAM variable=cartoon_color depth=64

    // Grayscale outputs
    pixel_stream gray_internal("gray_internal");
    pixel_stream gray_out("gray_out");
    pixel_stream to_median("to_median");
    #pragma HLS STREAM variable=gray_internal depth=64
    #pragma HLS STREAM variable=gray_out depth=64
    #pragma HLS STREAM variable=to_median depth=64

    // Median outputs
    pixel_stream median_internal("median_internal");
    pixel_stream median_out("median_out");
    pixel_stream to_threshold("to_threshold");
    #pragma HLS STREAM variable=median_internal depth=64
    #pragma HLS STREAM variable=median_out depth=64
    #pragma HLS STREAM variable=to_threshold depth=64

    // Threshold output
    pixel_stream thresh_out("thresh_out");
    #pragma HLS STREAM variable=thresh_out depth=64

    // ========================================================================
    // CANONICAL DATAFLOW: Only function calls, NO conditionals
    // ========================================================================

    // 1. Input conversion
    axis_to_pixel(src, in_pix);

    // 2. Initial 3-way split: passthrough + bilateral path + grayscale path
    triplicate_stream(in_pix, raw_passthrough, to_bilateral, to_grayscale);

    // 3. BILATERAL FILTER (color path) - always runs
    bilateral_filter(to_bilateral, bilateral_internal);
    
    // 4. Split bilateral output: one for MODE_BILATERAL, one for MODE_FULL_CARTOON
    duplicate_stream(bilateral_internal, bilateral_out, cartoon_color);

    // 5. GRAYSCALE conversion - always runs
    grayscale(to_grayscale, gray_internal);
    
    // 6. Split grayscale: one for MODE_GRAYSCALE, one for further processing
    duplicate_stream(gray_internal, gray_out, to_median);

    // 7. MEDIAN BLUR - always runs
    median_blur(to_median, median_internal);
    
    // 8. Split median: one for MODE_MEDIAN, one for threshold
    duplicate_stream(median_internal, median_out, to_threshold);

    // 9. ADAPTIVE THRESHOLD - always runs
    adaptive_threshold(to_threshold, thresh_out);

    // 10. OUTPUT SELECTION - reads all streams, selects based on mode
    output_selector_6way(
        raw_passthrough,  // Mode 0
        bilateral_out,    // Mode 1
        gray_out,         // Mode 2
        median_out,       // Mode 3
        thresh_out,       // Mode 4 (also used in Mode 5 for masking)
        cartoon_color,    // Mode 5 color
        out_pix,
        mode_sel
    );

    // 11. Output conversion
    pixel_to_axis(out_pix, dst);
}

// Stream wrapper for testbench
void stream(pixel_stream &src, pixel_stream &dst, int frame) {
    (void)frame;
    axis_stream axis_src("axis_src");
    axis_stream axis_dst("axis_dst");

    uint32_t mode = MODE_GRAYSCALE;

    pixel_to_axis(src, axis_src);
    cartoonize_pipeline_sel(axis_src, axis_dst, mode);
    axis_to_pixel(axis_dst, dst);
}