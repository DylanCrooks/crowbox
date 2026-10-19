// microSD storage: public interface.
#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#define STORAGE_MOUNT_POINT "/sdcard"

// Powers the card, mounts it as FAT at STORAGE_MOUNT_POINT, and starts the JPEG encoder.
esp_err_t storage_init(void);

// Encodes one RGB565 camera frame to JPEG and saves it as a new numbered file.
// The hardware encoder reads rgb565 by DMA, so it must be cache-line aligned:
// allocate it with heap_caps_aligned_alloc(64, CAMERA_FRAME_BYTES, MALLOC_CAP_SPIRAM).
// *photo_id (may be NULL) is set to the number in the file name.
esp_err_t storage_save_frame(const uint8_t *rgb565, size_t len, uint32_t *photo_id);
