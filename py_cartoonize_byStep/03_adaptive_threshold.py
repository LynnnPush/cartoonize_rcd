import cv2
import numpy as np

def adaptive_threshold_hls_model(img, max_value=255, ksize=7, C=7):
    """
    Unpacked version of cv2.adaptiveThreshold(..., cv2.ADAPTIVE_THRESH_MEAN_C, ...).
    Logic: Compare pixel to (Local_Mean - C).
    """
    height, width = img.shape
    output = np.zeros((height, width), dtype=np.uint8)
    pad = ksize // 2
    
    # Area of the window for mean calculation
    area = ksize * ksize

    for y in range(pad, height - pad):
        for x in range(pad, width - pad):
            local_sum = 0
            
            # Calculate sum of neighborhood
            for ky in range(-pad, pad + 1):
                for kx in range(-pad, pad + 1):
                    local_sum += int(img[y + ky, x + kx])
            
            # Calculate Mean (Integer division)
            local_mean = local_sum // area
            
            # Threshold Logic
            pixel = img[y, x]
            threshold_val = local_mean - C
            
            # THRESH_BINARY logic:
            if pixel > threshold_val:
                output[y, x] = max_value
            else:
                output[y, x] = 0

    return output

def main():
    img = cv2.imread("step2_blurred.jpg", cv2.IMREAD_GRAYSCALE)
    if img is None: return

    print("Running Adaptive Threshold HLS Model...")
    edges = adaptive_threshold_hls_model(img, max_value=255, ksize=7, C=7)
    
    cv2.imwrite("step3_edges.jpg", edges)
    print("Saved step3_edges.jpg")

if __name__ == "__main__":
    main()