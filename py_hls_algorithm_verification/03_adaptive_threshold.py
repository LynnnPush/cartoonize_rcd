import cv2
import numpy as np

def adaptive_threshold_optimized(img, max_value=255, ksize=7, C=7):
    """
    HLS Streaming Model with Optimized Sliding Window Sum.
    Complexity reduced from O(K^2) to O(K) per pixel.
    """
    height, width = img.shape
    output = np.zeros((height, width), dtype=np.uint8)

    # --- HARDWARE BUFFERS ---
    # 1. Line Buffer: Stores K-1 rows of pixels.
    line_buffer = np.zeros((ksize - 1, width), dtype=np.uint8)
    
    # 2. Window Buffer: The active KxK pixel kernel.
    window_buffer = np.zeros((ksize, ksize), dtype=np.uint8)
    
    # 3. Column Sum Buffer (Optimization): 
    #    Stores the pre-calculated sum of each column currently in the window.
    #    Size K (e.g., 7).
    col_sums_buffer = np.zeros(ksize, dtype=np.int32)

    # State variable for the running sum of the entire window
    current_window_sum = 0
    
    # Constants
    pad = ksize // 2
    area = ksize * ksize
    
    print(f"Streaming Optimized Adaptive Threshold (Sliding Sum, K={ksize})...")

    # --- STREAMING LOOP ---
    for y in range(height):
        
        # Reset running sum mechanisms at the start of every row
        # (In hardware, this happens naturally via HLS control signals)
        current_window_sum = 0
        col_sums_buffer.fill(0)
        
        for x in range(width):
            
            # 1. READ & UPDATE LINE BUFFER
            new_pixel = img[y, x]
            
            # Shift window pixels left
            window_buffer[:, :-1] = window_buffer[:, 1:]
            
            # Get the column from line buffer (past rows)
            col_from_buffer = line_buffer[:, x].copy()
            
            # Update window right-most column
            window_buffer[:-1, -1] = col_from_buffer
            window_buffer[-1, -1] = new_pixel
            
            # Update Line Buffer (Store new_pixel for future rows)
            line_buffer[:-1, x] = line_buffer[1:, x]
            line_buffer[-1, x] = new_pixel

            # --- OPTIMIZED SUMMATION LOGIC ---
            
            # A. Calculate sum of the NEW column (entering from right)
            #    (In hardware, this is an adder chain of K elements)
            new_col_sum = np.sum(window_buffer[:, -1])
            
            # B. Identify sum of the OLD column (leaving to the left)
            #    We retrieve this from our history buffer before we overwrite it.
            #    col_sums_buffer[0] is the left-most column sum.
            old_col_sum = col_sums_buffer[0]
            
            # C. Update the Total Window Sum
            #    New Sum = Old Sum + New Column - Old Column
            current_window_sum = current_window_sum + new_col_sum - old_col_sum
            
            # D. Update Column Sum Buffer
            #    Shift left, and put new column sum at the end
            col_sums_buffer[:-1] = col_sums_buffer[1:]
            col_sums_buffer[-1] = new_col_sum

            # --- THRESHOLD LOGIC ---
            # Valid only after buffer is primed
            if y >= ksize - 1 and x >= ksize - 1:
                
                # We already have the sum! No loops needed here.
                local_mean = current_window_sum // area
                
                # Center pixel is at [pad, pad]
                center_pixel = window_buffer[pad, pad]
                
                threshold_val = local_mean - C
                
                out_y = y - pad
                out_x = x - pad
                
                if center_pixel > threshold_val:
                    output[out_y, out_x] = max_value
                else:
                    output[out_y, out_x] = 0

    return output

def main():
    # Update to your actual path
    input_path = "D:\\PracticeProject\\TUD_RClab\\cartoonize_rcd\\py_cartoonize_byStep\\output\\step2_median_blur_delft_2.png"
    img = cv2.imread(input_path, cv2.IMREAD_GRAYSCALE)
    
    if img is None:
        print(f"Error: Could not read input image at {input_path}")
        return

    # Run optimized model
    edges = adaptive_threshold_optimized(img, max_value=255, ksize=7, C=7)
    
    # Save
    output_path = "D:\\PracticeProject\\TUD_RClab\\cartoonize_rcd\\py_cartoonize_byStep\\output\\step3_adaptThresh_optimized_delft_2.png"
    cv2.imwrite(output_path, edges)
    print(f"Saved optimized result to {output_path}")

if __name__ == "__main__":
    main()