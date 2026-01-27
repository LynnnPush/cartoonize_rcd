import cv2
import time
import psutil
import os
# Ensure your helper file is named 'utils.py' as discussed
from utils import create_output_dir, log_performance, load_image

STAGE_NAME = "05_complete_pipeline"

# === PATH SETUP ===
# Get the folder where this script is located
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# Input: Pointing to the original image in input_opencv
INPUT_PATH = os.path.join(SCRIPT_DIR, "input_opencv", "delft.png")

# Output: Saving to output_opencv/05_complete_pipeline
OUTPUT_DIR = os.path.join(SCRIPT_DIR, "output_opencv", STAGE_NAME)

if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

process = psutil.Process(os.getpid())

def run_pipeline():
    print(f"[{STAGE_NAME}] Starting full pipeline on: {INPUT_PATH}")
    
    # 1. Load Original Image
    img = load_image(INPUT_PATH)
    
    # Start Timer for the WHOLE pipeline
    start = time.time()
    
    # ---------------------------------------------------------
    # Step 1: Grayscale
    # ---------------------------------------------------------
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    
    # ---------------------------------------------------------
    # Step 2: Median Blur (Noise Reduction)
    # Using k=11 from your original notebook
    # ---------------------------------------------------------
    gray_blur = cv2.medianBlur(gray, 11)
    
    # ---------------------------------------------------------
    # Step 3: Adaptive Threshold (Edge Mask)
    # Using blockSize=13, C=5 from your original notebook
    # ---------------------------------------------------------
    edges = cv2.adaptiveThreshold(
        gray_blur,
        maxValue=255,
        adaptiveMethod=cv2.ADAPTIVE_THRESH_MEAN_C,
        thresholdType=cv2.THRESH_BINARY,
        blockSize=13,
        C=5
    )
    
    # ---------------------------------------------------------
    # Step 4: Bilateral Filter (Color Smoothing)
    # Using d=11, sigma=350 from your original notebook
    # Note: This is the most computationally expensive step
    # ---------------------------------------------------------
    bil_blurred = cv2.bilateralFilter(
        img,
        d=11,
        sigmaColor=350,
        sigmaSpace=350
    )
    
    # ---------------------------------------------------------
    # Step 5: Combine (Bitwise AND)
    # Masking the smoothed color image with the edge mask
    # ---------------------------------------------------------
    cartoon = cv2.bitwise_and(bil_blurred, bil_blurred, mask=edges)
    
    end = time.time()
    
    # ---------------------------------------------------------
    # Save Result and Log
    # ---------------------------------------------------------
    output_filename = "final_cartoon_full_pipeline.png"
    save_path = os.path.join(OUTPUT_DIR, output_filename)
    cv2.imwrite(save_path, cartoon)
    
    log_performance(STAGE_NAME, output_filename, start, end, process)
    print(f"[{STAGE_NAME}] Success! Saved to: {save_path}")

if __name__ == "__main__":
    run_pipeline()