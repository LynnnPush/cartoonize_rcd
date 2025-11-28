import cv2
import numpy as np

def bitwise_and_mask_hls_model(src, mask):
    """
    Unpacked version of cv2.bitwise_and(src, src, mask=edges).
    Logic: If mask is black (0), output is black. Else output is src.
    """
    height, width, channels = src.shape
    output = np.zeros_like(src)

    for y in range(height):
        for x in range(width):
            # Check mask value (Assuming single channel mask)
            mask_val = mask[y, x]
            
            if mask_val > 0:
                # Copy color pixel
                output[y, x, 0] = src[y, x, 0] # B
                output[y, x, 1] = src[y, x, 1] # G
                output[y, x, 2] = src[y, x, 2] # R
            else:
                # Edge detected -> Set to Black
                output[y, x, 0] = 0
                output[y, x, 1] = 0
                output[y, x, 2] = 0

    return output

def main():
    # Load results from previous steps
    color_img = cv2.imread("step4_bilateral.jpg")
    edge_mask = cv2.imread("step3_edges.jpg", cv2.IMREAD_GRAYSCALE)
    
    if color_img is None or edge_mask is None:
        print("Missing inputs from previous steps.")
        return

    # Resize checks (just in case padding made them different sizes)
    # In HLS, streams are synchronized, so sizes must match exactly.
    h, w = color_img.shape[:2]
    edge_mask = cv2.resize(edge_mask, (w, h))

    print("Running Combination HLS Model...")
    cartoon = bitwise_and_mask_hls_model(color_img, edge_mask)
    
    cv2.imwrite("final_cartoon.jpg", cartoon)
    print("Saved final_cartoon.jpg")

if __name__ == "__main__":
    main()