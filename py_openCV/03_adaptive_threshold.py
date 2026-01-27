import cv2
import time
import psutil
import os
from utils import create_output_dir, log_performance, load_image

STAGE_NAME = "03_adaptive_threshold"
# Picking one "best" result from previous stage as input
INPUT_PATH = "output_opencv/02_median_blur/median_k7.png" 
OUTPUT_DIR = create_output_dir(STAGE_NAME)
process = psutil.Process(os.getpid())

def run_stage():
    # Load Input
    img = load_image(INPUT_PATH)
    
    # cv2.imread loads as BGR by default. We must convert to Gray for adaptiveThreshold.
    if len(img.shape) == 3:
        img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    
    # Parameters to test
    # blockSize must be odd
    block_sizes = [9, 11, 13, 15] 
    c_values = [2, 5, 9, 12]

    for bs in block_sizes:
        for c in c_values:
            start = time.time()
            
            # Process: img = median-blurred gray scale image.
            edges = cv2.adaptiveThreshold(
                img,
                maxValue=255,
                adaptiveMethod=cv2.ADAPTIVE_THRESH_MEAN_C,
                thresholdType=cv2.THRESH_BINARY,
                blockSize=bs,
                C=c
            )
            
            end = time.time()
            
            filename = f"thresh_bs{bs}_c{c}.png"
            cv2.imwrite(os.path.join(OUTPUT_DIR, filename), edges)
            
            log_performance(STAGE_NAME, filename, start, end, process)

if __name__ == "__main__":
    run_stage()