#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "crowbox.h"

// Logs a crow visit starting (PIR tripped for the first time this visit)
void log_visit_start(training_stage_t stage);

// Logs a crow visit ending, with how long the crow was present
void log_visit_end(int64_t duration_ms);

// Logs a deposit attempt — weight in grams, whether it passed validation
void log_deposit(int32_t weight_grams, bool valid);

// Logs a reward being dispensed — which tier and which hoppers fired
void log_reward(reward_type_t type);

//Logs a state changing
void log_state_change(crowbox_state_t from, crowbox_state_t to);

//no arg version 
void log_state_change_lite(void);

// Logs the stage advancing
void log_stage_change(training_stage_t from, training_stage_t to, uint32_t deposit_count);

// Logs a hopper running empty
void log_hopper_empty(uint8_t hopper_id);

// Logs system boot — useful to detect unexpected resets in the field
void log_boot(training_stage_t stage, uint32_t successful_deposit_count);

// Logs a sensor or hardware fault
void log_error(const char *module, const char *detail);
