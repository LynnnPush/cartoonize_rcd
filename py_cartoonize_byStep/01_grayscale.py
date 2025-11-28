import cv2
import numpy as np

def rgb2gray_hls_model(img):
    """
    Unpacked version of cv2.cvtColor(img, cv2.COLOR_BGR2GRAY).
    Uses fixed-point approximation for hardware friendliness:
    Gray = (R * 76 + G * 150 + B * 29) >> 8
    """
    height, width, channels = img.shape
    # Create empty output array
    gray_img = np.zeros((height, width), dtype=np.uint8)

    # CRITICAL STEP: Iterate over every pixel
    for y in range(height):
        for x in range(width):
            # Access pixel (OpenCV uses BGR order)
            b = int(img[y, x, 0])
            g = int(img[y, x, 1])
            r = int(img[y, x, 2])

            # Standard Luminance formula: 0.299*R + 0.587*G + 0.114*B
            # HLS Optimization: Use integer math (x256 approx)
            # 0.299 * 256 ~= 76
            # 0.587 * 256 ~= 150
            # 0.114 * 256 ~= 29
            gray_val = (r * 76 + g * 150 + b * 29) >> 8
            
            # Clamp value to 0-255 just in case
            if gray_val > 255: gray_val = 255
            
            gray_img[y, x] = gray_val

    return gray_img

def main():
    input_path = "D:\\PracticeProject\\TUD_RClab\\cartoonize_rcd\\py_cartoonize_byStep\\input\\Things-to-do-in-Delft.jpg"
    img = cv2.imread(input_path)
    if img is None:
        print("Image not found.")
        return

    print("Running Grayscale HLS Model...")
    gray = rgb2gray_hls_model(img)
    
    output_path = "D:\\PracticeProject\\TUD_RClab\\cartoonize_rcd\\py_cartoonize_byStep\\output\\step1_gray.jpg"
    cv2.imwrite(output_path, gray)
    print(f"Saved {output_path}")

if __name__ == "__main__":
    main()