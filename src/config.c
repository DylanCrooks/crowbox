#include "config.h"
#include "crowbox.h"

crowbox_config_t g_config;

crowbox_config_t g_config = {
    .stage = STAGE_1A_DISCOVERY,
    .successful_deposit_count = 0,
    .departure_debounce_ms = 30000,
    .stage1_validation_strictness = 0,
    .auto_advance_stage = false,
};
//load from NVS
void config_init(){
};

const stage_config_t STAGE_CONFIGS[STAGE_COUNT] = {
    [STAGE_1A_DISCOVERY]  = {100, 100,  0, true},
    [STAGE_1B_PRESET_75]  = { 75, 100,  0, true},
    [STAGE_1C_PRESET_50]  = { 50, 100,  0, true},
    [STAGE_1D_PRESET_25]  = { 25, 100,  0, true},
    [STAGE_1E_NO_PRESET]  = {  0, 100,  0, false},
    [STAGE_2_SHAPING]     = {  0, 100,  0, false},
    [STAGE_3A_HVT_75]     = {  0,  75, 25, false},
    [STAGE_3B_HVT_50]     = {  0,  50, 25, false},
    [STAGE_3C_HVT_25]     = {  0,  25, 25, false},
    [STAGE_3D_HVT_GONE]   = {  0,   0, 25, false},
};

const size_t STAGE_CONFIGS_COUNT = STAGE_COUNT;

_Static_assert(
    sizeof(STAGE_CONFIGS) / sizeof(STAGE_CONFIGS[0]) == STAGE_COUNT,
    "STAGE_CONFIGS must have one entry per training_stage_t value"
);
