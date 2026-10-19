// Preprocessing: public interface.
// Plain C, no ESP-IDF: also builds and tests on a laptop (see vision/test/).
#pragma once

#include <stdint.h>

// RGB565 frame -> 8-bit grayscale, shrunk by `scale` in each direction (each output
// pixel is the average of a scale x scale block). out holds (w/scale) * (h/scale) bytes.
void preprocess_gray(const uint8_t *rgb565, int w, int h, int scale, uint8_t *out);

// RGB565 frame -> center square crop, resized to size x size, RGB888 (3 bytes per pixel).
// Input for the trained model (v2). out holds size * size * 3 bytes.
void preprocess_model_input(const uint8_t *rgb565, int w, int h, int size, uint8_t *out_rgb);
