// Classifier v2: trained int8 model via ESP-DL (C++ library, hence .cpp).
//
// Loads /sdcard/model.espdl, made on the laptop by vision/training (train.py, then
// quantize.py). With no file there, classify_model_init() returns false and vision.c keeps
// using the blob classifier, so deploying or rolling back a model is copying or deleting
// one file on the SD card. No reflash.
//
// Per frame: ESP-DL's ImagePreprocessor crops the center square of the RGB565 frame,
// resizes it to the model's 224x224 input (nearest neighbor), normalizes with the ImageNet
// mean/std and quantizes to int8, all in one pass. The model runs; its 3 outputs are
// dequantized, softmaxed, and the top class becomes the result.
#include <sys/stat.h>
#include <algorithm>
#include <array>
#include <cmath>
#include "esp_log.h"
#include "dl_model_base.hpp"
#include "dl_image_preprocessor.hpp"
#include "preprocess.h"     // CAMERA_RGB565_SWAP_BYTES
#include "storage.h"        // STORAGE_MOUNT_POINT
#include "classify.h"

#define MODEL_PATH          STORAGE_MOUNT_POINT "/model.espdl"
#define MODEL_NUM_CLASSES   3   // CLASS_NONE, CLASS_COIN, CLASS_OTHER: output index == UART_class_enum

static const char *TAG = "classify_model";

// Must match MEAN / STD in vision/training/common.py (here x 255: ESP-DL works in 0-255).
static const std::array<float, 3> kMean = {123.675f, 116.28f, 103.53f};
static const std::array<float, 3> kStd = {58.395f, 57.12f, 57.375f};

static dl::Model *s_model = nullptr;
static dl::image::ImagePreprocessor *s_preprocessor = nullptr;

static void unload(void)
{
    delete s_preprocessor;
    delete s_model;
    s_preprocessor = nullptr;
    s_model = nullptr;
}

bool classify_model_init(void)
{
    struct stat st;
    if (stat(MODEL_PATH, &st) != 0) {
        ESP_LOGI(TAG, "no %s", MODEL_PATH);
        return false;
    }

    s_model = new dl::Model(MODEL_PATH, fbs::MODEL_LOCATION_IN_SDCARD);
    if (s_model->get_inputs().empty()) {
        ESP_LOGE(TAG, "%s didn't load (not an .espdl from quantize.py?)", MODEL_PATH);
        unload();
        return false;
    }
    dl::TensorBase *output = s_model->get_output();
    if (output == nullptr || output->get_size() != MODEL_NUM_CLASSES) {
        ESP_LOGE(TAG, "model has %d outputs, expected %d (one per class)",
                 output ? output->get_size() : 0, MODEL_NUM_CLASSES);
        unload();
        return false;
    }

    s_preprocessor = new dl::image::ImagePreprocessor(s_model, kMean, kStd);
    ESP_LOGI(TAG, "loaded %s (%ld KB)", MODEL_PATH, (long)(st.st_size / 1024));
    return true;
}

bool classify_model(const uint8_t *rgb565, int w, int h, classify_result_t *out)
{
    if (s_model == nullptr || rgb565 == nullptr || out == nullptr) {
        return false;
    }

    dl::image::img_t img = {
        .data = (void *)rgb565,
        .width = (uint16_t)w,
        .height = (uint16_t)h,
        .pix_type = CAMERA_RGB565_SWAP_BYTES ? dl::image::DL_IMAGE_PIX_TYPE_RGB565BE
                                             : dl::image::DL_IMAGE_PIX_TYPE_RGB565LE,
    };
    // Center square, {x0, y0, x1, y1}: the same crop as device_crop_resize() in common.py.
    int side = std::min(w, h);
    int x0 = (w - side) / 2;
    int y0 = (h - side) / 2;
    s_preprocessor->preprocess(img, {x0, y0, x0 + side, y0 + side});

    s_model->run();

    // The output is int8 (int16 with quantize.py --int16-head) with a power-of-two scale;
    // assign() into a float tensor dequantizes.
    dl::TensorBase *quantized = s_model->get_output();
    dl::TensorBase logits(quantized->shape, nullptr, 0, dl::DATA_TYPE_FLOAT);
    if (!logits.assign(quantized)) {
        ESP_LOGE(TAG, "unexpected output type %s", dl::dtype_to_string(quantized->dtype));
        return false;
    }
    const float *z = (const float *)logits.data;

    // Softmax -> probabilities; the top one is the class, its probability the confidence.
    float z_max = *std::max_element(z, z + MODEL_NUM_CLASSES);
    float sum = 0.0f;
    int best = 0;
    for (int i = 0; i < MODEL_NUM_CLASSES; i++) {
        sum += std::exp(z[i] - z_max);
        if (z[i] > z[best]) {
            best = i;
        }
    }
    float p = std::exp(z[best] - z_max) / sum;

    out->cls = (uint8_t)best;
    out->confidence = (uint8_t)std::lround(p * 99.0f);
    return true;
}
