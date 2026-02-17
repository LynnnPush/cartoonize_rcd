# `select_mode` Branch — Runtime Output Selection

> **This branch extends the main cartoonization pipeline with a mode-selectable variant.** For the full project overview, pipeline algorithm details, resource utilisation, and getting-started instructions, please see the [README on `master`](../../tree/master).

---

## What This Branch Adds

The `master` branch implements a fixed pipeline that always outputs the final cartoon composite. This branch replaces the fixed output with a **runtime-selectable 6-way output multiplexer**, so you can visualise every intermediate processing stage on the live HDMI monitor — without resynthesising the bitstream.

### Available Output Modes

| Mode | Value | Output Description |
|---|---|---|
| `MODE_NONE` | 0 | Raw passthrough (unprocessed input) |
| `MODE_BILATERAL` | 1 | Bilateral-filtered colour image |
| `MODE_GRAYSCALE` | 2 | Grayscale conversion only |
| `MODE_MEDIAN` | 3 | Grayscale + median blur |
| `MODE_ADAPTIVE_THRESHOLD` | 4 | Binary edge mask |
| `MODE_FULL_CARTOON` | 5 | Full cartoon composite (bilateral colour with edge overlay) |

Modes are defined in [`hls/cartoonize_pipeline.h`](hls/cartoonize_pipeline.h) as the `FilterMode` enum.

---

## Key Design Decisions

**Unconditional execution with output selection.** Rather than conditionally bypassing stages, every filter runs continuously. A final multiplexer selects the output. This preserves II = 1 throughout the pipeline at the cost of higher resource utilisation (all stages are always active).

**Stream duplication via helper functions.** The input stream is first triplicated (`triplicate_stream`) to feed the raw passthrough, bilateral, and grayscale paths. Intermediate results are further duplicated (`duplicate_stream`) where they serve as both a mode output and the input to a downstream stage. Each helper is pipelined at II = 1 and marked `INLINE off` so HLS treats them as distinct dataflow processes.

**AXI-Lite mode register.** The `mode` argument of `cartoonize_pipeline_sel` is exposed as an `s_axilite` port, making it writable from the ARM processor at runtime. Changing the mode takes effect on the next pixel — no reset or reconfiguration needed.

---

## Top-Level Function

The synthesis entry point on this branch is:

```cpp
void cartoonize_pipeline_sel(axis_stream &src, axis_stream &dst, uint32_t mode);
```

with the following HLS interface pragmas:

```cpp
#pragma HLS INTERFACE axis port=src
#pragma HLS INTERFACE axis port=dst
#pragma HLS INTERFACE s_axilite port=mode
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS DATAFLOW disable_start_propagation
```

Make sure to select `cartoonize_pipeline_sel` (not the master branch's `cartoonize_pipeline_v2`) as the top-level function in your Vitis HLS project settings before synthesis.

---

## Switching Modes from Python

After deploying the bitstream and `.hwh` to the PYNQ board, the mode register is accessible through MMIO or the register map. Using the register map (preferred):

```python
from pynq import Overlay

ol = Overlay("/path/to/cartoonize_overlay.bit")
reg = ol.cartoonize_pipeline_sel_0.register_map

# Switch to grayscale-only output
reg.mode = 2

# Switch to full cartoon
reg.mode = 5

# Raw passthrough (no processing visible)
reg.mode = 0
```

Or equivalently via MMIO, using the `mode` register offset from the HLS-generated address map:

```python
from pynq import MMIO

cartoon_mmio = MMIO(BASE_ADDR, 0x10000)
cartoon_mmio.write(MODE_OFFSET, 3)  # median blur output
```

The video stream is never interrupted — the mode change is picked up on the very next pixel.

---

## Vivado Integration Notes

The block-design integration is the same as on `master` (AXI-Stream ports between `hdmi_in` and `axi_vdma`, 142 MHz video clock domain), with one addition: the `s_axi_control` port of the IP must be connected to an available AXI master on the existing interconnect (e.g. `M10_AXI`). Vivado's address editor should automatically assign an address range; verify it and note the base address for use in your Jupyter notebook.

---

## Resource Overhead vs. Master

Because all stages execute unconditionally and additional stream duplication logic is required, the mode-selectable variant uses more FPGA resources than the fixed pipeline on `master`. The overhead is acceptable for development, debugging, and demonstration, where being able to inspect each stage in real time is valuable. For production deployment where only the final cartoon output is needed, the `master` branch's single-version pipeline is recommended for its lower resource footprint.

---

## Files Changed Relative to Master

| File | Changes |
|---|---|
| `hls/cartoonize_pipeline.h` | Added `FilterMode` enum, `cartoonize_pipeline_sel` declaration |
| `hls/cartoonize_pipeline.cpp` | Added `triplicate_stream`, `duplicate_stream`, `output_selector_6way`, and the `cartoonize_pipeline_sel` top-level function |

The individual filter stage sources (`grayscale`, `bilateral_filter`, `median_blur`, `adaptive_threshold`) are unchanged from `master`.