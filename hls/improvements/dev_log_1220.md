# 🛠️ Dev Log: Cartoonize Pipeline HLS Fix

**Date:** December 20, 2025
**Module:** `cartoonize_pipeline`
**Topic:** Fixing "Black Output" / Pipeline Stall in Free-Running Kernel

## 1. Issue Description

The HLS video processing pipeline was outputting **all black frames** (zero data) despite correct functional logic in C simulation.

* **Symptoms:**
* Output stream contained valid protocol signals (`tlast`, `tuser`) but data was consistently 0.
* In specific test scenarios (removing processing tasks), a simple `passthrough` worked, but replacing it with `grayscale` caused the output to freeze after a few lines.


* **Configuration:** Top-level interface set to `ap_ctrl_none` (free-running), using `#pragma HLS DATAFLOW` for task parallelism.

## 2. The Solution

**Change:** updated the DATAFLOW pragma in `cartoonize_pipeline.cpp`.

**Before (Stalling):**

```cpp
#pragma HLS DATAFLOW

```

**After (Functional):**

```cpp
#pragma HLS DATAFLOW disable_start_propagation

```

## 3. Technical Root Cause Analysis

### A. The Protocol Conflict (`ap_ctrl_none` vs. Handshake)

* **The Conflict:** The standard `DATAFLOW` pragma (without options) creates a **Control-Driven** architecture. It generates internal `ap_start`/`ap_done` "handshake" signals between functions to enforce task ordering. It expects a master start signal to kick off the chain.
* **The Failure:** Since our top-level interface is `ap_ctrl_none`, there is no master start signal. The internal tasks were waiting for a "Go" token that never arrived, causing the pipeline to idle at the first stage (deadlock).

### B. The "Partial Output" Anomaly (Grayscale vs. Passthrough)

During debugging, replacing the processing logic with a simple wire-passthrough seemed to work, while `grayscale` froze after a few lines.

* **Mechanism:** Standard `DATAFLOW` inserts shallow **Start FIFOs** (depth ~2-3) alongside deep **Data FIFOs** (depth 64).
* **Why `grayscale` failed:** The `axis_to_pixel` task is faster than `grayscale`. It flooded the shallow Start FIFO with tokens. Once the Start FIFO filled up, the upstream task stalled/deadlocked because the downstream task (`grayscale`) was too slow to clear the tokens.
* **Why `passthrough` worked:** The passthrough logic had near-zero latency, consuming Start Tokens instantly. This prevented the Start FIFO from overflowing, masking the underlying architectural flaw.

## 4. Why `disable_start_propagation` Works

Adding this option switches the internal synchronization from **Control-Driven** to **Data-Driven**.

* It removes the internal Start/Done handshakes and Start FIFOs.
* Tasks now execute purely based on **FIFO Pressure**:
* *Has Input Data?* → Run.
* *Has Output Space?* → Run.


* This effectively decouples the control plane, allowing the `ap_ctrl_none` interface to stream pixels continuously without artificial handshakes stalling the pipe.

## 5. Key Takeaway for Team

> For any **pure streaming kernel** using `ap_ctrl_none` (where we process infinite streams pixel-by-pixel rather than batch/frame-based memory operations), **always** use `#pragma HLS DATAFLOW disable_start_propagation`. Standard start propagation provides no value for streams and introduces high risk of deadlock due to token backpressure.