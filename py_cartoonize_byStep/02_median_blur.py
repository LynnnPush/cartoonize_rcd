import cv2
import numpy as np

K_SIZE = 5  # Kernel size for median blur, must be odd

def hls_bubble_sort_kernel(window_list):
    """ Same hardware-friendly sort as before """
    buffer = list(window_list) # Copy to avoid reference issues
    N = len(buffer)
    for i in range(N - 1):
        for j in range(N - 1 - i):
            if buffer[j] > buffer[j+1]:
                buffer[j], buffer[j+1] = buffer[j+1], buffer[j]
    return buffer

def median_blur_line_buffer_model(img, ksize=K_SIZE):
    height, width = img.shape
    output = np.zeros((height, width), dtype=np.uint8)
    
    # --- HARDWARE STRUCTURES ---
    # 1. Line Buffer: Stores (K-1) full rows of the image.
    #    In FPGA, this maps to Block RAM (BRAM).
    line_buffer = np.zeros((ksize - 1, width), dtype=np.uint8)
    
    # 2. Window Buffer: The active KxK kernel.
    #    In FPGA, this maps to Registers / Flip-Flops (LUTs).
    window_buffer = np.zeros((ksize, ksize), dtype=np.uint8)
    
    # Calculate padding to center the output
    center_offset = ksize // 2
    
    print(f"Streaming Image with Line Buffer (Size: {ksize-1} x {width})...")

    # --- STREAMING LOOP (Pixel by Pixel) ---
    # We iterate over every pixel in the image ONCE.
    for y in range(height):
        for x in range(width):
            
            # 1. READ: Fetch only ONE new pixel from main memory
            new_pixel = img[y, x]
            
            # 2. SHIFT WINDOW LEFT: Make room for new column
            #    (In hardware, this is a parallel shift)
            window_buffer[:, :-1] = window_buffer[:, 1:]
            
            # 3. UPDATE LINE BUFFER & FILL WINDOW COLUMN
            #    We retrieve the vertical column at position 'x' from the line buffer
            #    and append the new_pixel to the bottom of that column.
            
            # Top parts of the column come from the line buffer
            col_from_buffer = line_buffer[:, x].copy()
            
            # Update the window's right-most column
            # First (K-1) elements come from buffer
            window_buffer[:-1, -1] = col_from_buffer
            # Last element is the new pixel we just read
            window_buffer[-1, -1] = new_pixel
            
            # 4. UPDATE LINE BUFFER MEMORY
            #    Shift the column in the line buffer up to make room for new_pixel
            #    so it's ready for the NEXT row (y+1).
            line_buffer[:-1, x] = line_buffer[1:, x] # Shift older rows up
            line_buffer[-1, x] = new_pixel           # Store new pixel at bottom
            
            # 5. COMPUTE: Valid Data Check
            #    We can only compute a valid median once we have processed enough
            #    rows and columns to fill the buffer initially.
            if y >= ksize - 1 and x >= ksize - 1:
                
                # Flatten the 2D window to 1D array for sorting
                # (In hardware this is just wiring)
                flat_window = window_buffer.flatten()
                
                # Sort and Pick Median
                sorted_window = hls_bubble_sort_kernel(flat_window)
                median_val = sorted_window[len(sorted_window) // 2]
                
                # Write to output (adjusting for the delay caused by buffering)
                # The result corresponds to the pixel at the center of the window.
                out_y = y - center_offset
                out_x = x - center_offset
                output[out_y, out_x] = median_val

    return output

def main():
    # Use output from first step as input
    input_path = "D:\\PracticeProject\\TUD_RClab\\cartoonize_rcd\\py_cartoonize_byStep\\output\\step1_gray.jpg"
    img = cv2.imread(input_path, cv2.IMREAD_GRAYSCALE)
    
    if img is None:
        print("Grayscale image not found - run step 01 first")
        return

    print("Running Median Blur...")
    blurred = median_blur_line_buffer_model(img)
    
    # Save in same output folder with new filename
    output_path = "D:\\PracticeProject\\TUD_RClab\\cartoonize_rcd\\py_cartoonize_byStep\\output\\step2_median_blur.jpg"
    cv2.imwrite(output_path, blurred)
    print(f"Saved {output_path}")

if __name__ == "__main__":
    main()