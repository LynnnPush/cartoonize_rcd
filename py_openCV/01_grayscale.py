import cv2
import time
import psutil
import os
from utils import create_output_dir, log_performance, load_image

# Setup
STAGE_NAME = "01_grayscale"
INPUT_PATH = "input_opencv/delft.png"  # <-- CHANGE THIS to your image name
OUTPUT_DIR = create_output_dir(STAGE_NAME)
process = psutil.Process(os.getpid())

def run_stage():
    # Load Input
    img = load_image(INPUT_PATH)
    
    # Start Timer
    start = time.time()
    
    # Process
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    
    # End Timer
    end = time.time()
    
    # Save Output
    output_filename = "gray_base.png"
    cv2.imwrite(os.path.join(OUTPUT_DIR, output_filename), gray)
    
    # Log
    log_performance(STAGE_NAME, output_filename, start, end, process)

if __name__ == "__main__":
    run_stage()