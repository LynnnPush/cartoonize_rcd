import cv2
import time
import psutil
import os
from utils import create_output_dir, log_performance, load_image

STAGE_NAME = "04_bilateral_filter"
INPUT_PATH = "input_opencv/delft.png" # Uses original color image
OUTPUT_DIR = create_output_dir(STAGE_NAME)
process = psutil.Process(os.getpid())

def run_stage():
    img = load_image(INPUT_PATH)
    
    # Parameters to test
    # d: Diameter of pixel neighborhood
    # sigmaColor: Filter sigma in color space (larger = more cartoonish color flattening)
    # sigmaSpace: Filter sigma in coordinate space
    
    params = [
        (9, 75, 75),
        (11, 150, 150),
        (15, 200, 200),
        (9, 300, 300) # Extreme smoothing
    ]

    for (d, sc, ss) in params:
        start = time.time()
        
        # Process
        filtered = cv2.bilateralFilter(img, d=d, sigmaColor=sc, sigmaSpace=ss)
        
        end = time.time()
        
        filename = f"bilateral_d{d}_sc{sc}_ss{ss}.png"
        cv2.imwrite(os.path.join(OUTPUT_DIR, filename), filtered)
        
        log_performance(STAGE_NAME, filename, start, end, process)

if __name__ == "__main__":
    run_stage()