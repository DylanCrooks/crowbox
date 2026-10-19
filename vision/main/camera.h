// Camera: public interface.
#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#define CAMERA_WIDTH        1280
#define CAMERA_HEIGHT       960
#define CAMERA_FRAME_BYTES  (CAMERA_WIDTH * CAMERA_HEIGHT * 2)  // RGB565: 2 bytes per pixel

// Starts the video stack, opens the OV5647, sets 1280x960 RGB565, maps the driver's buffers.
esp_err_t camera_init(void);

// Captures one settled frame and copies it into out (allocate CAMERA_FRAME_BYTES in PSRAM).
// *len is set to the bytes written.
esp_err_t camera_capture(uint8_t *out, size_t out_size, size_t *len);
