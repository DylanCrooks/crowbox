// THROWAWAY bench test: takes photos without the main ESP32 (see capture_test.h).
// Uses only the public camera/storage/preprocess/classify functions, on its own buffers.
// Nothing sends jobs over the UART on the bench, so it never competes with the vision worker.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "driver/jpeg_encode.h"
#include "driver/gpio.h"
#include "camera.h"
#include "storage.h"
#include "preprocess.h"
#include "classify.h"
#include "capture_test.h"

#define CAPTURE_TEST_START_DELAY_S  5       // let boot logs settle
#define CAPTURE_TEST_PERIOD_S       2
#define CAPTURE_TEST_COUNT          25      // then stop (SD space)
#define CAPTURE_TEST_GRAY_SCALE     4       // same as vision.c: 1280x960 -> 320x240
#define CAPTURE_TEST_GRAY_W         (CAMERA_WIDTH / CAPTURE_TEST_GRAY_SCALE)
#define CAPTURE_TEST_GRAY_H         (CAMERA_HEIGHT / CAPTURE_TEST_GRAY_SCALE)
#define CAPTURE_TEST_ROW_STEP       8       // rows sampled for the byte-order check

static const char *TAG = "capture_test";

static const char *class_name(uint8_t cls)
{
    switch (cls) {
    case CLASS_NONE:  return "none";
    case CLASS_COIN:  return "coin";
    case CLASS_OTHER: return "other";
    default:          return "?";
    }
}

static inline uint8_t gray_of(uint16_t p)
{
    uint8_t r = (p >> 11) & 0x1F, g = (p >> 5) & 0x3F, b = p & 0x1F;
    return (uint8_t)((77 * (r << 3) + 150 * (g << 2) + 29 * (b << 3)) >> 8);
}

// A real photo is mostly smooth, so neighbouring pixels have similar brightness. Read with the
// wrong byte order, every pixel's bits are scrambled and neighbours jump around. Sums
// |gray[x] - gray[x-1]| over sampled rows for both orders; the smaller sum is the right order.
static void check_byte_order(const uint8_t *f, uint8_t *mean_out)
{
    uint64_t rough_le = 0, rough_be = 0, sum = 0, n = 0;
    for (int y = 0; y < CAMERA_HEIGHT; y += CAPTURE_TEST_ROW_STEP) {
        const uint8_t *row = f + (size_t)y * CAMERA_WIDTH * 2;
        int prev_le = -1, prev_be = -1;
        for (int x = 0; x < CAMERA_WIDTH; x++) {
            uint8_t lo = row[2 * x], hi = row[2 * x + 1];
            int g_le = gray_of((uint16_t)(hi << 8 | lo));
            int g_be = gray_of((uint16_t)(lo << 8 | hi));
            if (prev_le >= 0) {
                rough_le += abs(g_le - prev_le);
                rough_be += abs(g_be - prev_be);
            }
            prev_le = g_le;
            prev_be = g_be;
            sum += CAMERA_RGB565_SWAP_BYTES ? g_be : g_le;
            n++;
        }
    }
    *mean_out = (uint8_t)(sum / n);
    bool le_smoother = rough_le <= rough_be;
    bool configured_le = !CAMERA_RGB565_SWAP_BYTES;
    ESP_LOGI(TAG, "byte order: roughness LE=%" PRIu64 " BE=%" PRIu64 " -> frame looks %s; "
             "CAMERA_RGB565_SWAP_BYTES=%d is %s", rough_le, rough_be,
             le_smoother ? "little-endian" : "big-endian", CAMERA_RGB565_SWAP_BYTES,
             le_smoother == configured_le ? "RIGHT" : "WRONG (flip it in preprocess.h)");
}

// Writes len bytes from buf to path; logs OK/FAIL and the time.
static void sd_write_probe(const char *label, const char *path, const void *buf, size_t len)
{
    int64_t t0 = esp_timer_get_time();
    FILE *f = fopen(path, "wb");
    size_t n = f ? fwrite(buf, 1, len, f) : 0;
    int closed = f ? fclose(f) : -1;
    ESP_LOGW(TAG, "SD probe %-22s %6u bytes: %s (%lld ms)", label, (unsigned)len,
             (f && n == len && closed == 0) ? "OK" : "FAIL", (esp_timer_get_time() - t0) / 1000);
}

// Which SD writes work: FATFS-buffered small write, big write from internal RAM, big from PSRAM.
static void sd_probes(void)
{
    static const char small[] = "crowbox sd probe\n";
    sd_write_probe("small (FATFS buffer)", STORAGE_MOUNT_POINT "/probe1.txt", small, sizeof(small) - 1);

    const size_t big = 64 * 1024;
    uint8_t *internal = heap_caps_aligned_alloc(64, big, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    uint8_t *psram = heap_caps_aligned_alloc(128, big, MALLOC_CAP_SPIRAM);
    if (internal) {
        memset(internal, 0xA5, big);
        sd_write_probe("64K from internal RAM", STORAGE_MOUNT_POINT "/probe2.bin", internal, big);
        free(internal);
    }
    if (psram) {
        memset(psram, 0x5A, big);
        sd_write_probe("64K from PSRAM", STORAGE_MOUNT_POINT "/probe3.bin", psram, big);
        free(psram);
    }
}

static void capture_test_task(void *arg)
{
    uint8_t *frame = heap_caps_aligned_alloc(128, CAMERA_FRAME_BYTES, MALLOC_CAP_SPIRAM);
    uint8_t *gray = heap_caps_malloc(CAPTURE_TEST_GRAY_W * CAPTURE_TEST_GRAY_H, MALLOC_CAP_SPIRAM);
    uint8_t *ref = heap_caps_malloc(CAPTURE_TEST_GRAY_W * CAPTURE_TEST_GRAY_H, MALLOC_CAP_SPIRAM);
    if (frame == NULL || gray == NULL || ref == NULL) {
        ESP_LOGE(TAG, "buffer allocation failed");
        vTaskDelete(NULL);
    }

    vTaskDelay(pdMS_TO_TICKS(CAPTURE_TEST_START_DELAY_S * 1000));
    sd_probes();
    ESP_LOGW(TAG, "THROWAWAY TEST: %d captures, one every %d s. Frame 1 = empty-pit reference: "
             "keep the view empty for it.", CAPTURE_TEST_COUNT, CAPTURE_TEST_PERIOD_S);

    for (int i = 1; i <= CAPTURE_TEST_COUNT; i++) {
        int64_t t0 = esp_timer_get_time();
        size_t len = 0;
        esp_err_t err = camera_capture(frame, CAMERA_FRAME_BYTES, &len);
        int64_t t_cap = esp_timer_get_time();
        if (i == 1) {
            ESP_LOGW(TAG, "SD probes right after the first camera capture (before any JPEG encode):");
            sd_probes();
            // Same JPEG settings as storage.c, with this test's own engine and buffer.
            jpeg_encoder_handle_t enc = NULL;
            jpeg_encode_engine_cfg_t ecfg = {.timeout_ms = 200};
            jpeg_encode_memory_alloc_cfg_t mcfg = {.buffer_direction = JPEG_ENC_ALLOC_OUTPUT_BUFFER};
            size_t out_size = 0;
            uint8_t *out = jpeg_alloc_encoder_mem(512 * 1024, &mcfg, &out_size);
            if (jpeg_new_encoder_engine(&ecfg, &enc) == ESP_OK && out != NULL) {
                jpeg_encode_cfg_t jcfg = {
                    .width = CAMERA_WIDTH, .height = CAMERA_HEIGHT,
                    .src_type = JPEG_ENCODE_IN_FORMAT_RGB565,
                    .sub_sample = JPEG_DOWN_SAMPLING_YUV420, .image_quality = 80,
                };
                uint32_t jlen = 0;
                esp_err_t jerr = jpeg_encoder_process(enc, &jcfg, frame, CAMERA_FRAME_BYTES, out, out_size, &jlen);
                ESP_LOGW(TAG, "test JPEG encode: %s, %u bytes. SD probes after it:", esp_err_to_name(jerr), (unsigned)jlen);
                sd_probes();
                if (jerr == ESP_OK) {
                    sd_write_probe("the JPEG itself", STORAGE_MOUNT_POINT "/probe4.jpg", out, jlen);
                }
                jpeg_del_encoder_engine(enc);
            } else {
                ESP_LOGE(TAG, "couldn't create the test JPEG encoder");
            }
            free(out);
        }
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "capture %d: camera_capture failed (%s)", i, esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(CAPTURE_TEST_PERIOD_S * 1000));
            continue;
        }

        uint8_t mean;
        check_byte_order(frame, &mean);

        preprocess_gray(frame, CAMERA_WIDTH, CAMERA_HEIGHT, CAPTURE_TEST_GRAY_SCALE, gray);
        classify_result_t res = {0};
        bool ok;
        if (i == 1) {
            for (int p = 0; p < CAPTURE_TEST_GRAY_W * CAPTURE_TEST_GRAY_H; p++) {
                ref[p] = gray[p];
            }
            ok = true;
            res.cls = CLASS_NONE;
        } else {
            ok = classify_blob(gray, ref, &res);
        }
        int64_t t_cls = esp_timer_get_time();

        uint32_t photo_id = 0;
        err = storage_save_frame(frame, CAMERA_FRAME_BYTES, &photo_id);
        int64_t t_save = esp_timer_get_time();

        ESP_LOGI(TAG, "capture %d: %u bytes, mean brightness %u/255 | %s%s conf %u | %s img%05" PRIu32
                 ".jpg | capture %lld ms, classify %lld ms, save %lld ms",
                 i, (unsigned)len, mean,
                 i == 1 ? "REFERENCE " : "", ok ? class_name(res.cls) : "classify failed", res.confidence,
                 err == ESP_OK ? "saved" : "SAVE FAILED", photo_id,
                 (t_cap - t0) / 1000, (t_cls - t_cap) / 1000, (t_save - t_cls) / 1000);

        vTaskDelay(pdMS_TO_TICKS(CAPTURE_TEST_PERIOD_S * 1000));
    }
    ESP_LOGW(TAG, "done: pull the SD card and look at the img*.jpg files");
    free(frame);
    free(gray);
    free(ref);
    vTaskDelete(NULL);
}

void capture_test_start(void)
{
    xTaskCreate(capture_test_task, "capture_test", 8192, NULL, 4, NULL);
}
