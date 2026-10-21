// Vision worker: takes jobs from the link, runs the photo pipeline, sends the final reply.
//   CLASSIFY:    capture -> classify (trained model if /sdcard/model.espdl exists, else blob)
//                -> reply -> save JPEG + a line in /sdcard/results.csv (training labels)
//   CAPTURE_REF: capture -> grayscale -> keep as the empty-pit background -> reply
//                -> save JPEG, logged as a known "none" example
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "link.h"
#include "camera.h"
#include "storage.h"
#include "preprocess.h"
#include "classify.h"
#include "power.h"
#include "vision.h"

#define VISION_TASK_STACK_SIZE  (8192)  // ESP-DL's run() goes deeper than the blob classifier
#define VISION_TASK_PRIORITY    (5)     // below LINK_TASK_PRIORITY
// DMA buffers must start on a cache line. The P4's L2 line is 64 B by default and can be set
// to 128 B (Espressif's ML examples do); 128 covers both.
#define VISION_BUF_ALIGN        (128)

#define VISION_GRAY_SCALE       (4)     // 1280x960 -> 320x240 for the blob classifier
#define VISION_GRAY_W           (CAMERA_WIDTH / VISION_GRAY_SCALE)
#define VISION_GRAY_H           (CAMERA_HEIGHT / VISION_GRAY_SCALE)

#define VISION_RESULTS_PATH     STORAGE_MOUNT_POINT "/results.csv"

static const char *TAG = "vision";

static bool s_camera_ok = false;
static bool s_storage_ok = false;
static bool s_blob_ok = false;
static bool s_model_ok = false;

static uint8_t *s_frame;        // latest capture, RGB565, 1280x960
static uint8_t *s_gray;         // latest capture, grayscale, 320x240
static uint8_t *s_ref_gray;     // empty-pit background from CAPTURE_REF, grayscale
static bool s_have_reference = false;

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

    s_model_ok = s_storage_ok && classify_model_init();     // the model file lives on the SD card
    ESP_LOGI(TAG, "classifier: %s", s_model_ok ? "model" : "blob");
}

// Appends "photo_id,class,confidence,classifier" for sort_photos.py on the laptop.
// classifier is "model" or "blob" (a guess, to review), "reference" (the pit was known empty:
// a free, correct CLASS_NONE example), or "none" (photo kept, nothing classified it).
static void log_result(uint32_t photo_id, const UART_msg_t *reply, const char *classifier)
{
    struct stat st;
    bool is_new = (stat(VISION_RESULTS_PATH, &st) != 0);
    FILE *f = fopen(VISION_RESULTS_PATH, "a");
    if (f == NULL) {
        return;
    }
    if (is_new) {
        fprintf(f, "photo_id,class,confidence,classifier\n");
    }
    if (reply->type == MSG_RESULT) {
        fprintf(f, "%lu,%u,%u,%s\n", (unsigned long)photo_id, reply->arg0, reply->arg1, classifier);
    } else if (strcmp(classifier, "reference") == 0) {
        fprintf(f, "%lu,%u,99,reference\n", (unsigned long)photo_id, CLASS_NONE);
    } else {
        fprintf(f, "%lu,,,none\n", (unsigned long)photo_id);
    }
    fflush(f);
    fsync(fileno(f));
    fclose(f);
}

// Fills *reply. Returns true if s_frame holds a fresh capture worth saving.
static bool handle_classify(UART_msg_t *reply, const char **classifier)
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
        classified = classify_model(s_frame, CAMERA_WIDTH, CAMERA_HEIGHT, &result);
        *classifier = "model";
    }
    if (!classified && s_blob_ok) {
        preprocess_gray(s_frame, CAMERA_WIDTH, CAMERA_HEIGHT, VISION_GRAY_SCALE, s_gray);
        classified = classify_blob(s_gray, s_have_reference ? s_ref_gray : NULL, &result);
        *classifier = "blob";
    }

    if (!classified) {
        reply->type = MSG_ERROR;
        reply->arg0 = ERR_CLASSIFIER_FAILED;    // main falls back to weight
        return true;                            // the photo is still worth keeping
    }
    reply->type = MSG_RESULT;
    reply->arg0 = result.cls;
    reply->arg1 = result.confidence;
    return true;
}

// Returns true if s_frame holds the empty-pit capture (also saved: training data for "none").
static bool handle_capture_ref(UART_msg_t *reply)
{
    size_t len = 0;
    if (!s_camera_ok || camera_capture(s_frame, CAMERA_FRAME_BYTES, &len) != ESP_OK) {
        reply->type = MSG_ERROR;
        reply->arg0 = ERR_CAMERA_FAILED;
        return false;
    }
    preprocess_gray(s_frame, CAMERA_WIDTH, CAMERA_HEIGHT, VISION_GRAY_SCALE, s_ref_gray);
    s_have_reference = true;
    reply->type = MSG_OK;
    return true;
}

static void vision_task(void *arg)
{
    while (1) {
        UART_msg_t job;
        if (!link_get_request(&job, portMAX_DELAY)) {
            continue;
        }

        UART_msg_t reply = {
            .type = MSG_OK,
            .ID = job.ID,
            .arg0 = 0,
            .arg1 = 0,
        };
        bool save_photo = false;
        bool sleep_after = false;
        const char *classifier = "none";

        switch (job.type) {
        case MSG_CLASSIFY:
            save_photo = handle_classify(&reply, &classifier);
            break;

        case MSG_CAPTURE_REF:
            save_photo = handle_capture_ref(&reply);
            classifier = "reference";
            break;

        case MSG_SLEEP:
            // Handled here rather than in the link task, so it runs after any job in front of
            // it (an SD save) has finished. Reply OK first, then sleep below.
            sleep_after = true;
            break;

        default:
            ESP_LOGW(TAG, "unexpected job type %u", job.type);
            continue;
        }

        // Reply first: the main ESP32 is waiting on it, and the SD write takes a while.
        ESP_LOGI(TAG, "job type=%u id=%u -> reply type=%u arg0=%u arg1=%u (%s)",
                 job.type, job.ID, reply.type, reply.arg0, reply.arg1, classifier);
        link_send(&reply);

        // A failed save costs a training photo, not the answer.
        if (save_photo && s_storage_ok) {
            uint32_t photo_id;
            if (storage_save_frame(s_frame, CAMERA_FRAME_BYTES, &photo_id) == ESP_OK) {
                log_result(photo_id, &reply, classifier);
            }
        }

        if (sleep_after) {
            power_enter_deep_sleep();   // doesn't return
        }
        if (job.type == MSG_CLASSIFY || job.type == MSG_CAPTURE_REF) {
            link_job_done();    // reply sent and photo saved: let the link task admit the next job
        }
    }
}

void vision_start(void)
{
    xTaskCreate(vision_task, "vision", VISION_TASK_STACK_SIZE, NULL, VISION_TASK_PRIORITY, NULL);
}
