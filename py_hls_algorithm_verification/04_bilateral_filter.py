import cv2
import numpy as np
import math


def gaussian(diff, sigma):
    return math.exp(-(diff * diff) / (2 * sigma * sigma))


def bilateral_filter_hls_model(img, d=5, sigmaColor=60, sigmaSpace=60):
    height, width, ch = img.shape
    output = np.zeros_like(img)

    center = d // 2

    # ------------ LINE BUFFER (BRAM) ------------
    line_buffer = np.zeros((d - 1, width, ch), dtype=np.uint8)

    # ------------ WINDOW BUFFER (REGS) ----------
    window = np.zeros((d, d, ch), dtype=np.uint8)

    # ------------ PRECOMPUTE SPATIAL ------------
    spatial = np.zeros((d, d))
    for i in range(d):
        for j in range(d):
            dy = i - center
            dx = j - center
            spatial[i, j] = gaussian(math.sqrt(dx * dx + dy * dy), sigmaSpace)

    for y in range(height):
        if y % 20 == 0:
            print(f"Row {y+1}/{height}")

        for x in range(width):

            new_px = img[y, x]

            # 1) SHIFT WINDOW LEFT
            window[:, :-1, :] = window[:, 1:, :]

            # 2) COLUMN FROM LINE BUFFER + NEW PIXEL
            if y >= 1:
                col = line_buffer[:, x, :].copy()
                window[:-1, -1, :] = col
            else:
                window[:-1, -1, :] = 0

            window[-1, -1, :] = new_px

            # 3) UPDATE LINE BUFFER (same as median-blur model)
            if d > 1:
                line_buffer[:-1, x, :] = line_buffer[1:, x, :]
                line_buffer[-1, x, :] = new_px

            # 4) VALID OUTPUT?
            if y >= d - 1 and x >= d - 1:

                out_y = y - center
                out_x = x - center

                for c in range(ch):
                    center_val = window[center, center, c]

                    norm = 0.0
                    wsum = 0.0

                    for i in range(d):
                        for j in range(d):
                            neigh = window[i, j, c]
                            diff = float(neigh) - float(center_val)

                            w_color = gaussian(diff, sigmaColor)
                            w = spatial[i, j] * w_color

                            wsum += neigh * w
                            norm += w

                    output[out_y, out_x, c] = int(wsum / (norm + 1e-8))

    return output


def main():
    img = cv2.imread("D:\\PracticeProject\\TUD_RClab\\cartoonize_rcd\\py_cartoonize_byStep\\input\\Things-to-do-in-Delft.jpg")
    if img is None:
        print("Image not found")
        return

    filtered = bilateral_filter_hls_model(
        img,
        d=5,
        sigmaColor=75,
        sigmaSpace=75
    )

    output_path = "D:\\PracticeProject\\TUD_RClab\\cartoonize_rcd\\py_cartoonize_byStep\\output\\step4_bilateral.jpg"
    cv2.imwrite(output_path, filtered)
    print(f"Saved {output_path}")


if __name__ == "__main__":
    main()
