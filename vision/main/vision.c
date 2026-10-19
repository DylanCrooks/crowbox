// Vision worker: takes jobs from the link, runs the photo pipeline, sends the final reply.
//   CLASSIFY:    capture -> grayscale -> classify (model if loaded, else blob) -> reply -> save JPEG
//   CAPTURE_REF: capture -> grayscale -> keep as the empty-pit background -> reply
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "link.h"
#include "camera.h"
#include "storage.h"
#include "preprocess.h"
#include "classify.h"
#include "vision.h"

#define VISION_TASK_STACK_SIZE  (4096)  // will grow once the model runs here
#define VISION_TASK_PRIORITY    (5)     // below LINK_TASK_PRIORITY
#define VISION_BUF_ALIGN        (64)    // cache line; the JPEG encoder DMAs from the frame

#define VISION_GRAY_SCALE       (4)     // 1280x960 -> 320x240 for the blob classifier
#define VISION_GRAY_W           (CAMERA_WIDTH / VISION_GRAY_SCALE)
#define VISION_GRAY_H           (CAMERA_HEIGHT / VISION_GRAY_SCALE)
#define VISION_MODEL_INPUT_SIZE (224)   // MobileNetV2 input, v2

static const char *TAG = "vision";

static bool s_camera_ok = false;
static bool s_storage_ok = false;
static bool s_blob_ok = false;
static bool s_model_ok = false;

static uint8_t *s_frame;        // latest capture, RGB565, 1280x960
static uint8_t *s_gray;         // latest capture, grayscale, 320x240
static uint8_t *s_ref_gray;     // empty-pit background from CAPTURE_REF, grayscale
static bool s_have_reference = false;
static uint8_t *s_model_in;     // 224x224 RGB888, only when a model is loaded

void vision_init(void)
{
    s_frame = heap_caps_aligned_alloc(VISION_BUF_ALIGN, CAMERA_FRAME_BYTES, MALLOC_CAP_SPIRAM);
    s_gray = heap_caps_malloc(VISION_GRAY_W * VISION_GRAY_H, MALLOC_CAP_SPIRAM);
    s_ref_gray = heap_caps_malloc(VISION_GRAY_W * VISION_GRAY_H, MALLOC_CAP_SPIRAM);
    if (s_frame == NULL || s_gray == NULL || s_ref_gray == NULL) {
        ESP_LOGE(TAG, "frame buffer allocation failed (is PSRAM enabled?)");
        return;
    }

    s_camera_ok = (camera_init() == ESP_OK);
    if (!s_camera_ok) {
        ESP_LOGE(TAG, "camera init failed; jobs will get ERR_CAMERA_FAILED");
    }

    s_storage_ok = (storage_init() == ESP_OK);
    if (!s_storage_ok) {
        ESP_LOGE(TAG, "SD init failed; photos won't be saved");
    }

    s_blob_ok = classify_blob_init(VISION_GRAY_W, VISION_GRAY_H);
    if (!s_blob_ok) {
        ESP_LOGE(TAG, "blob classifier buffers failed");
    }

    s_model_ok = classify_model_init();
    if (s_model_ok) {
        s_model_in = heap_caps_malloc(VISION_MODEL_INPUT_SIZE * VISION_MODEL_INPUT_SIZE * 3, MALLOC_CAP_SPIRAM);
        s_model_ok = (s_model_in != NULL);
    }
    ESP_LOGI(TAG, "classifier: %s", s_model_ok ? "model" : "blob");
}

// Fills *reply. Returns true if s_frame holds a fresh capture worth saving.
static bool handle_classify(UART_msg_t *reply)
{
    size_t len = 0;
    if (!s_camera_ok || camera_capture(s_frame, CAMERA_FRAME_BYTES, &len) != ESP_OK) {
        reply->type = MSG_ERROR;
        reply->arg0 = ERR_CAMERA_FAILED;
        return false;
    }

    classify_result_t result;
    bool classified = false;
    if (s_model_ok) {
        preprocess_model_input(s_frame, CAMERA_WIDTH, CAMERA_HEIGHT, VISION_MODEL_INPUT_SIZE, s_model_in);
        classified = classify_model(s_model_in, &result);
    }
    if (!classified && s_blob_ok) {
        preprocess_gray(s_frame, CAMERA_WIDTH, CAMERA_HEIGHT, VISION_GRAY_SCALE, s_gray);
        classified = classify_blob(s_gray, s_have_reference ? s_ref_gray : NULL, &result);
    }

    if (!classified) {
        reply->type = MSG_ERROR;
        reply->arg0 = ERR_CLASSIFIER_FAILED;    // main falls back to weight
        return true;                        // the photo is still worth keeping
    }
    reply->type = MSG_RESULT;
    reply->arg0 = result.cls;
    reply->arg1 = result.confidence;
    return true;
}

static void handle_capture_ref(UART_msg_t *reply)
{
    size_t len = 0;
    if (!s_camera_ok || camera_capture(s_frame, CAMERA_FRAME_BYTES, &len) != ESP_OK) {
        reply->type = MSG_ERROR;
        reply->arg0 = ERR_CAMERA_FAILED;
        return;
    }
    preprocess_gray(s_frame, CAMERA_WIDTH, CAMERA_HEIGHT, VISION_GRAY_SCALE, s_ref_gray);
    s_have_reference = true;
    reply->type = MSG_OK;
}

static void vision_task(void *arg)
{
    while (1) {
        UART_msg_t job;
        if (!link_get_job(&job, portMAX_DELAY)) {
            continue;
        }

        UART_msg_t reply = {
            .type = MSG_OK,
            .ID = job.ID,
            .arg0 = 0,
            .arg1 = 0,
        };
        bool save_photo = false;

        switch (job.type) {
        case MSG_CLASSIFY:
            save_photo = handle_classify(&reply);
            break;

        case MSG_CAPTURE_REF:
            handle_capture_ref(&reply);
            break;

        default:
            ESP_LOGW(TAG, "unexpected job type %u", job.type);
            continue;
        }

        // Reply first: the main ESP32 is waiting on it, and the SD write takes a while.
        ESP_LOGI(TAG, "job type=%u id=%u -> reply type=%u arg0=%u arg1=%u",
                 job.type, job.ID, reply.type, reply.arg0, reply.arg1);
        link_send(&reply);

        // A failed save costs a training photo, not the answer.
        if (save_photo && s_storage_ok) {
            uint32_t photo_id;
            if (storage_save_frame(s_frame, CAMERA_FRAME_BYTES, &photo_id) == ESP_OK) {
                ESP_LOGI(TAG, "job id=%u -> photo %lu (class %u)", job.ID,
                         (unsigned long)photo_id, reply.arg0);
            }
        }
    }
}

void vision_start(void)
{
    xTaskCreate(vision_task, "vision", VISION_TASK_STACK_SIZE, NULL, VISION_TASK_PRIORITY, NULL);
}
