import os
import time
import psutil
import csv
import cv2

# Ensure output directories exist
def create_output_dir(stage_name):
    path = os.path.join("output_opencv", stage_name)
    if not os.path.exists(path):
        os.makedirs(path)
    return path

# Performance Logger
def log_performance(stage_name, image_name, start_time, end_time, process):
    log_file = os.path.join("output_opencv", "performance_log.csv")
    
    # Calculate metrics
    duration_sec = end_time - start_time
    throughput = 1 / duration_sec if duration_sec > 0 else 0
    memory_usage_mb = process.memory_info().rss / (1024 * 1024)
    cpu_percent = process.cpu_percent()
    
    # Simple power estimation proxy (CPU usage * Time)
    # Real power requires hardware sensors, this is a relative metric.
    power_proxy_units = cpu_percent * duration_sec 

    file_exists = os.path.isfile(log_file)
    
    with open(log_file, mode='a', newline='') as file:
        writer = csv.writer(file)
        if not file_exists:
            writer.writerow(["Stage", "Image", "Resolution", "Latency (s)", "Throughput (fps)", "RAM (MB)", "CPU (%)", "Est. Energy (Units)"])
        
        writer.writerow([
            stage_name, 
            image_name, 
            "N/A",  # Can be filled if image object is passed
            f"{duration_sec:.4f}", 
            f"{throughput:.2f}", 
            f"{memory_usage_mb:.2f}", 
            f"{cpu_percent:.2f}",
            f"{power_proxy_units:.2f}"
        ])
    
    print(f"[{stage_name}] Saved: {image_name} | Time: {duration_sec:.4f}s | RAM: {memory_usage_mb:.1f}MB")

def load_image(path):
    img = cv2.imread(path)
    if img is None:
        raise FileNotFoundError(f"Could not find image at {path}")
    return img