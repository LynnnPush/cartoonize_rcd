import cv2
import numpy as np
import math

def gaussian(x, sigma):
    return (1.0 / (2 * math.pi * (sigma ** 2))) * math.exp(-(x ** 2) / (2 * (sigma ** 2)))

def bilateral_filter_hls_model(img, d, sigmaColor, sigmaSpace):
    """
    Unpacked version of cv2.bilateralFilter.
    Logic: Weighted sum based on Spatial Distance AND Color Difference.
    """
    height, width, channels = img.shape
    output = np.zeros_like(img)
    pad = d // 2
    
    # Pre-compute spatial gaussian weights to save time (HLS Lookup Table)
    space_weights = np.zeros((d, d))
    for i in range(d):
        for j in range(d):
            dist = math.sqrt((i - pad)**2 + (j - pad)**2)
            space_weights[i, j] = gaussian(dist, sigmaSpace)

    print("Processing Bilateral Filter (Warning: Very slow in pure Python)...")
    
    # Iterate pixels
    for y in range(pad, height - pad):
        if y % 10 == 0: print(f"Processing row {y}/{height}") # Progress bar
        for x in range(pad, width - pad):
            
            # We process 3 channels (B, G, R)
            for c in range(channels):
                pixel_val = img[y, x, c]
                norm_factor = 0.0
                pixel_sum = 0.0
                
                # Iterate Kernel
                for ky in range(d):
                    for kx in range(d):
                        neighbor_y = y - pad + ky
                        neighbor_x = x - pad + kx
                        neighbor_val = img[neighbor_y, neighbor_x, c]
                        
                        # 1. Spatial Weight (from pre-computed table)
                        w_space = space_weights[ky, kx]
                        
                        # 2. Color/Range Weight (Intensity difference)
                        # In HLS, this exp() is usually replaced by a pre-computed LUT 
                        # mapped to the range 0-255 diff.
                        diff = float(neighbor_val) - float(pixel_val)
                        w_color = gaussian(diff, sigmaColor)
                        
                        weight = w_space * w_color
                        
                        pixel_sum += neighbor_val * weight
                        norm_factor += weight
                
                # Normalize and assign
                output[y, x, c] = int(pixel_sum / norm_factor)
                
    return output

def main():
    # Load original colored image
    img = cv2.imread("input_image.jpg") 
    if img is None: return

    # For demonstration, we use smaller parameters than the notebook because
    # pure Python implementation is extremely slow.
    # Notebook used: d=10, sigma=250.
    filtered = bilateral_filter_hls_model(img, d=5, sigmaColor=75, sigmaSpace=75)
    
    cv2.imwrite("step4_bilateral.jpg", filtered)
    print("Saved step4_bilateral.jpg")

if __name__ == "__main__":
    main()