#pragma once
#include "crowbox.h"

typedef struct {
    training_stage_t  stage;
    uint32_t          successful_deposit_count;
    uint32_t          departure_debounce_ms;
    uint8_t           stage1_validation_strictness;
    bool              auto_advance_stage;
} crowbox_config_t;

extern crowbox_config_t g_config;

void config_init(void);
void config_save(void);

// Call after every successful (hvt-rewarded) deposit.
// Increments count, checks stage threshold, advances stage and saves if due.
void config_on_deposit_success(void);
