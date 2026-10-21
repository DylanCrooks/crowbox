// Classifier interface: classify(frame) -> label + confidence. Implemented by classify_blob.c or classify_model.cpp.
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "vision_protocol.h"    // UART_class_enum

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t cls;            // UART_class_enum
    uint8_t confidence;     // 0-99
} classify_result_t;

// ---- v1: classic CV (classify_blob.c). Plain C, no ESP-IDF: also tests on a laptop. ----

// Allocates work buffers for w x h grayscale images. Call once at boot.
bool classify_blob_init(int w, int h);

// gray: the deposit frame. ref: the empty-pit frame, or NULL if there isn't one yet
// (the floor's typical brightness then stands in for it). Both w x h, from preprocess_gray().
bool classify_blob(const uint8_t *gray, const uint8_t *ref, classify_result_t *out);

// ---- v2: trained int8 model via ESP-DL (classify_model.cpp), made by vision/training/. ----

// Loads /sdcard/model.espdl. False if there's no model file (then use the blob classifier).
// Call after storage_init() has mounted the card.
bool classify_model_init(void);

// rgb565: the full camera frame (w x h). The model crops and resizes it itself, exactly
// as vision/training/common.py does to the training photos.
bool classify_model(const uint8_t *rgb565, int w, int h, classify_result_t *out);

#ifdef __cplusplus
}
#endif
