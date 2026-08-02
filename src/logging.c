#include "logging.h"
#include "esp_log.h"

#define TAG "log"

// CLAUDE TODO: Phase 5 — replace ESP_LOGI stubs with Notecard JSON writes via notecard.c
// Each function should build a JSON payload and call notecard_send(payload).
// For now, all functions just log to serial so the data is visible during development.

void log_visit_start(int64_t timestamp_ms, training_stage_t stage) {
    ESP_LOGI(TAG, "visit_start  ts=%lld  stage=%d", timestamp_ms, stage);
}

void log_visit_end(int64_t timestamp_ms, uint32_t duration_ms) {
    ESP_LOGI(TAG, "visit_end  ts=%lld  duration=%lums", timestamp_ms, duration_ms);
}

void log_deposit(int64_t timestamp_ms, int32_t weight_grams, bool valid) {
    ESP_LOGI(TAG, "deposit  ts=%lld  weight=%ldg  valid=%d", timestamp_ms, weight_grams, valid);
}

void log_reward(int64_t timestamp_ms, reward_type_t type) {
    ESP_LOGI(TAG, "reward  ts=%lld  type=%d", timestamp_ms, type);
}

void log_stage_change(training_stage_t from, training_stage_t to, uint32_t deposit_count) {
    ESP_LOGI(TAG, "stage_change  from=%d  to=%d  deposits=%lu", from, to, deposit_count);
}

void log_hopper_empty(int64_t timestamp_ms, uint8_t hopper_id) {
    ESP_LOGI(TAG, "hopper_empty  ts=%lld  hopper=%d", timestamp_ms, hopper_id);
}

void log_boot(training_stage_t stage, uint32_t successful_deposit_count) {
    ESP_LOGI(TAG, "boot  stage=%d  total_deposits=%lu", stage, successful_deposit_count);
}

void log_error(const char *module, const char *detail) {
    ESP_LOGE(TAG, "error  module=%s  detail=%s", module, detail);
}
