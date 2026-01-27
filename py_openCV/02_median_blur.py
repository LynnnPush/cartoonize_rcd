import cv2
import time
import psutil
import os
from utils import create_output_dir, log_performance, load_image

STAGE_NAME = "02_median_blur"
INPUT_PATH = "output_opencv/01_grayscale/gray_base.png"
OUTPUT_DIR = create_output_dir(STAGE_NAME)
process = psutil.Process(os.getpid())

def run_stage():
    img = load_image(INPUT_PATH)
    
    # Parameters to test (Kernel sizes must be odd)
    kernel_sizes = [3, 5, 7, 9, 11, 13, 15]

    for k in kernel_sizes:
        start = time.time()
        
        # Process
        blurred = cv2.medianBlur(img, k)
        
        end = time.time()
        
        # Save
        filename = f"median_k{k}.png"
        cv2.imwrite(os.path.join(OUTPUT_DIR, filename), blurred)
        
        # Log
        log_performance(STAGE_NAME, filename, start, end, process)

if __name__ == "__main__":
    run_stage()