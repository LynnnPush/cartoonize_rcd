#include "adaptive_threshold.h"

void adaptive_threshold(pixel_stream &src, pixel_stream &dst)
{
    // Optimization: Pipeline the loop with Initiation Interval = 1
    #pragma HLS PIPELINE II=1

    // ----------------------------------------------------------------------
    // 1. STATE & BUFFERS
    // ----------------------------------------------------------------------
    
    // Position counters
    static uint16_t x = 0;
    static uint16_t y = 0;
    static uint8_t line_idx = 0; // Cyclic buffer index

    // A. Line Buffer: Stores K-1 rows of pixels
    //    Partitioned to allow reading a full column in one cycle.
    static uint8_t line_buffer[K_SIZE - 1][WIDTH];
    #pragma HLS ARRAY_PARTITION variable=line_buffer dim=1 complete // Partition row index (K_SIZE-1) for parallel read
    #pragma HLS BIND_STORAGE variable=line_buffer type=ram_2p impl=bram // FORCE the implementation to BRAM (RAM_2P or RAM_S2P)
    #pragma HLS DEPENDENCE variable=line_buffer inter false

    // B. Window Buffer: The active KxK pixel kernel
    static uint8_t window_buffer[K_SIZE][K_SIZE];
    #pragma HLS ARRAY_PARTITION variable=window_buffer complete dim=0

    // C. Column Sum Buffer (Optimization): 
    //    Stores the pre-calculated sum of columns currently in the window.
    //    Acts as a shift register of size K.
    static uint16_t col_sums_buffer[K_SIZE];
    #pragma HLS ARRAY_PARTITION variable=col_sums_buffer complete dim=0

    // State variable for the running sum of the entire window
    // Max value approx 255 * 49 = ~12,500, fits in uint16
    static uint16_t current_window_sum = 0;

    // ----------------------------------------------------------------------
    // 2. INPUT READ & SYNC
    // ----------------------------------------------------------------------
    
    pixel_data p_in;
    src >> p_in;

    // Handle Frame Start (TUSER)
    if (p_in.user) {
        x = 0;
        y = 0;
        line_idx = 0;
    }

    // Reset running sums at the start of every row (Python Logic)
    if (x == 0) {
        current_window_sum = 0;
        for (int i = 0; i < K_SIZE; i++) {
            #pragma HLS UNROLL
            col_sums_buffer[i] = 0;
        }
    }

    // Extract Grayscale Pixel (Assuming input is typically in R channel or Gray)
    uint8_t new_pixel = rgba2r(p_in.data);

    // ----------------------------------------------------------------------
    // 3. BUFFER MANAGEMENT (Line Buffer & Window Shift)
    // ----------------------------------------------------------------------

    // Shift window pixels left
    for(int i=0; i < K_SIZE; i++) {
        #pragma HLS UNROLL
        for(int j=0; j < K_SIZE - 1; j++) {
            #pragma HLS UNROLL
            window_buffer[i][j] = window_buffer[i][j+1];
        }
    }

    // Read from Line Buffer into Window (Rightmost column)
    // And update Line Buffer with new pixel
    if (x < WIDTH) {
        // We write the new pixel into the buffer for the *next* time we visit this X
        // We read the *old* pixels to form the column
        
        // 1. Fill top K-1 pixels of the new column from line buffer
        uint8_t col_bank[K_SIZE - 1];
        #pragma HLS ARRAY_PARTITION variable=col_bank complete dim=0
        for (int r = 0; r < K_SIZE - 1; r++) {
            #pragma HLS UNROLL
            col_bank[r] = line_buffer[r][x];
        }

        for (int i = 0; i < K_SIZE - 1; i++) {
            #pragma HLS UNROLL
            int idx = line_idx + i;
            if (idx >= (K_SIZE - 1)) idx -= (K_SIZE - 1);
            window_buffer[i][K_SIZE - 1] = col_bank[idx];
        }

        window_buffer[K_SIZE - 1][K_SIZE - 1] = new_pixel;
        line_buffer[line_idx][x] = new_pixel;
    }

    // ----------------------------------------------------------------------
    // 4. OPTIMIZED SUMMATION LOGIC (Sliding Sum)
    // ----------------------------------------------------------------------

    // A. Calculate sum of the NEW column (entering from right)
    //    HLS will map this to an adder tree
    uint16_t new_col_sum = 0;
    for (int i = 0; i < K_SIZE; i++) {
        #pragma HLS UNROLL
        new_col_sum += window_buffer[i][K_SIZE - 1];
    }

    // B. Identify sum of the OLD column (leaving to the left)
    //    Retrieve from history buffer index 0
    uint16_t old_col_sum = col_sums_buffer[0];

    // C. Update Total Window Sum
    //    New Sum = Old Sum + New Column - Old Column
    //    Note: We only do this math effectively.
    current_window_sum = current_window_sum + new_col_sum - old_col_sum;

    // D. Update Column Sum Buffer (Shift Left)
    for(int i = 0; i < K_SIZE - 1; i++) {
        #pragma HLS UNROLL
        col_sums_buffer[i] = col_sums_buffer[i+1];
    }
    col_sums_buffer[K_SIZE - 1] = new_col_sum;

    // ----------------------------------------------------------------------
    // 5. THRESHOLD LOGIC
    // ----------------------------------------------------------------------
    
    pixel_data p_out = p_in; // Copy metadata (user, last, etc)
    uint8_t result_pixel = 0;

    // Logic valid only after buffer is primed (y >= K-1 and x >= K-1)
    // Note: HLS latency means we process data continuously, but valid output
    // logically corresponds to the center pixel [pad, pad].
    
    if (y >= K_SIZE - 1 && x >= K_SIZE - 1) {
        
        // 1. Calculate Mean (Integer division)
        //    For K=7, Area=49. 
        uint8_t local_mean = current_window_sum / K_AREA;

        // 2. Get Center Pixel
        uint8_t center_pixel = window_buffer[K_PAD][K_PAD];

        // 3. Calculate Threshold (Handling underflow for uint)
        int threshold_val = (int)local_mean - C_CONST;
        
        // 4. Binarize
        if ((int)center_pixel > threshold_val) {
            result_pixel = MAX_VAL;
        } else {
            result_pixel = 0;
        }
    } else {
        // Keep mask open during warm-up so borders don't get forced to black
        result_pixel = MAX_VAL;
    }

    // Pack Result (Replicate to RGB for display consistency)
    p_out.data = r2rgba(result_pixel) | g2rgba(result_pixel) | b2rgba(result_pixel);
    
    // Write to Output
    dst << p_out;

    // ----------------------------------------------------------------------
    // 6. COORDINATE UPDATES
    // ----------------------------------------------------------------------
    
    if (p_in.last) {
        x = 0;
        y++;
        // Move circular buffer index
        line_idx++;
        if (line_idx >= (K_SIZE - 1)) line_idx = 0;
    } else {
        x++;
    }
}

// Optional standalone stream wrapper for testing this stage only
void adaptive_threshold_stream(pixel_stream &src, pixel_stream &dst, int frame)
{
    (void)frame;
    adaptive_threshold(src, dst);
}
