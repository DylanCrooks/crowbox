#include "crowbox.h"


typedef struct {
    training_stage_t  stage;
    uint32_t          successful_deposit_count;
    uint32_t          departure_debounce_ms;
    uint8_t           stage1_validation_strictness;
    bool              auto_advance_stage;  // can disable for manual control
} crowbox_config_t;