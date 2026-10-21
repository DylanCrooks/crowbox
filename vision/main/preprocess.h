// Preprocessing: public interface.
// Plain C, no ESP-IDF: also builds and tests on a laptop (see vision/test/).
#pragma once

#include <stdint.h>

// Byte order of each RGB565 pixel in the camera buffer. 0 = little-endian (low byte first,
// assumed ISP default). If grayscale photos come out as noise on the real board, set to 1.
// Used by preprocess.c (blob classifier) and classify_model.cpp (ESP-DL's RGB565LE/BE).
#define CAMERA_RGB565_SWAP_BYTES 0

// RGB565 frame -> 8-bit grayscale, shrunk by `scale` in each direction (each output
// pixel is the average of a scale x scale block). out holds (w/scale) * (h/scale) bytes.
void preprocess_gray(const uint8_t *rgb565, int w, int h, int scale, uint8_t *out);
