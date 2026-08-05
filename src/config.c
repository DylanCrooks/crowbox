#include "config.h"
#include "crowbox.h"
#include "logging.h"
#include "nvs_flash.h"
#include "nvs.h"

// Cumulative deposit thresholds to advance to the next stage.
// Index is the current stage — when count reaches this value, stage increments.
static const uint32_t STAGE_DEPOSIT_THRESHOLDS[STAGE_COUNT] = {
    [STAGE_1A_DISCOVERY] = 100,
    [STAGE_1B_PRESET_75] = 200,
    [STAGE_1C_PRESET_50] = 300,
    [STAGE_1D_PRESET_25] = 400,
    [STAGE_1E_NO_PRESET] = 500,
    [STAGE_2_SHAPING]    = 600,
    [STAGE_3A_HVT_75]    = 700,
    [STAGE_3B_HVT_50]    = 800,
    [STAGE_3C_HVT_25]    = 900,
    [STAGE_3D_HVT_GONE]  = UINT32_MAX,  // last stage — never advance
};

crowbox_config_t g_config = {
    .stage                       = STAGE_1A_DISCOVERY,
    .successful_deposit_count    = 0,
    .departure_debounce_ms       = 30000,
    .stage1_validation_strictness = 0,
    .auto_advance_stage          = false,
};

void config_init(void) {
    nvs_handle_t h;
    if (nvs_open("crowbox", NVS_READWRITE, &h) != ESP_OK) return;

    uint32_t count = 0;
    nvs_get_u32(h, "deposit_count", &count);  // stays 0 on first boot (NOT_FOUND)

    training_stage_t stage = STAGE_1A_DISCOVERY;
    size_t size = sizeof(training_stage_t);
    nvs_get_blob(h, "stage", &stage, &size);  // stays default on first boot

    nvs_close(h);
    g_config.successful_deposit_count = count;
    g_config.stage = stage;
}

void config_save(void) {
    nvs_handle_t h;
    if (nvs_open("crowbox", NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_u32(h, "deposit_count", g_config.successful_deposit_count);
    nvs_set_blob(h, "stage", &g_config.stage, sizeof(training_stage_t));
    nvs_commit(h);
    nvs_close(h);
}

void config_on_deposit_success(void) {
    g_config.successful_deposit_count++;

    if (g_config.auto_advance_stage && g_config.stage < STAGE_COUNT - 1) {
        if (g_config.successful_deposit_count >= STAGE_DEPOSIT_THRESHOLDS[g_config.stage]) {
            training_stage_t old_stage = g_config.stage;
            g_config.stage = (training_stage_t)(g_config.stage + 1);
            log_stage_change(old_stage, g_config.stage, g_config.successful_deposit_count);
        }
    }

    config_save();
}

const stage_config_t STAGE_CONFIGS[STAGE_COUNT] = {
    [STAGE_1A_DISCOVERY] = {100, 100,  0, true},
    [STAGE_1B_PRESET_75] = { 75, 100,  0, true},
    [STAGE_1C_PRESET_50] = { 50, 100,  0, true},
    [STAGE_1D_PRESET_25] = { 25, 100,  0, true},
    [STAGE_1E_NO_PRESET] = {  0, 100,  0, false},
    [STAGE_2_SHAPING]    = {  0, 100,  0, false},
    [STAGE_3A_HVT_75]    = {  0,  75, 25, false},
    [STAGE_3B_HVT_50]    = {  0,  50, 25, false},
    [STAGE_3C_HVT_25]    = {  0,  25, 25, false},
    [STAGE_3D_HVT_GONE]  = {  0,   0, 25, false},
};

const size_t STAGE_CONFIGS_COUNT = STAGE_COUNT;

_Static_assert(
    sizeof(STAGE_CONFIGS) / sizeof(STAGE_CONFIGS[0]) == STAGE_COUNT,
    "STAGE_CONFIGS must have one entry per training_stage_t value"
);
