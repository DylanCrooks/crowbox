#include "state_machine.h"
#include "event_bus.h"
#include "crowbox.h"
#include "config.h"
#include "pir.h"
#include "servo.h"
#include "gate.h"
#include "solenoid.h"
#include "weight.h"
#include "mcp23017.h"
#include "buzzer.h"
#include "pinout.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "logging.h"
#include "ir_beam.h"
#include "cv.h"
#include "sleep.h"
#include "esp_system.h"
#include <stdlib.h>


#define TAG "state_machine"

static crowbox_state_t s_current_state = STATE_IDLE;
static bool s_crow_departed = false;
static bool s_object_in_chamber = false;
static bool s_deposit_logged = false;
static bool s_object_in_trapdoor = false;
static bool s_reward_in_chamber = true; 
static bool s_reward_gate_up = false;
static int64_t s_visit_start_ms = 0;
static int32_t s_last_deposit_weight_g = 0;
static bool s_last_deposit_valid = false;


static const char *state_name(crowbox_state_t s) {
    switch (s) {
        case STATE_IDLE:                return "IDLE";
        case STATE_PIR_TRIPPED:         return "PIR_TRIPPED";
        case STATE_DEPOSIT_DETECTED:    return "DEPOSIT_DETECTED";
        case STATE_VALIDATING:          return "VALIDATING";
        case STATE_DISPENSING:          return "DISPENSING";
        case STATE_COOLDOWN:            return "COOLDOWN";
        case STATE_LOGGING:             return "LOGGING";
        case STATE_ERROR:               return "ERROR";
        default:                        return "UNKNOWN";
    }
}

static void transition_to(crowbox_state_t new_state) {
    ESP_LOGI(TAG, "state: %s -> %s", state_name(s_current_state), state_name(new_state));
    log_state_change_lite();
    s_current_state = new_state;
}

static void run_preset_sequence(void) {
    if (weight_reward_tray_is_empty()) {
            bool preset_hit = (rand() % 100) < STAGE_CONFIGS[g_config.stage].preset_chance_pct;
            if (preset_hit) {servo_doser_dispense(0);}
        }
}
static void run_trapdoor_sequence(void) {
    if (s_object_in_trapdoor) {
            solenoid_unlock();
            int64_t time_trapdoor_unlock = (esp_timer_get_time()/1000);
            ESP_LOGI(TAG,"time at trapdoor unlock: %lld", time_trapdoor_unlock);
            vTaskDelay(pdMS_TO_TICKS(150));
            servo_trapdoor_set_angle(90);
            vTaskDelay(pdMS_TO_TICKS(2000));
            servo_trapdoor_set_angle(0);
            vTaskDelay(pdMS_TO_TICKS(2000));
            solenoid_lock();
        }
}

//should communicate with Notecard here. 
static void run_logging_sequence(void) {
    int64_t ts_end = (esp_timer_get_time() / 1000);
    int64_t duration = ts_end - s_visit_start_ms;
    log_visit_end(duration);
    // reset all visit-level state for next visit
    s_object_in_trapdoor = false;
    s_crow_departed = false;
    s_object_in_chamber = false;
    s_deposit_logged = false;
    s_last_deposit_weight_g = 0;
    s_last_deposit_valid = false;
    s_reward_in_chamber = false;
    s_reward_gate_up = false;
}

static void run_cooldown_sequence(void) {
    if (gate_is_open()) { gate_close(); }
    run_trapdoor_sequence();
    run_preset_sequence();
    for (int i = 0; i < 4; i++) {
        if (ir_beam_hopper_is_empty(i)) {
            log_hopper_empty(i);
        }
    }
    transition_to(STATE_LOGGING);
    run_logging_sequence();
    transition_to(STATE_IDLE);
    sleep_enter_deep();
}

// handle_pir_tripped
// Called when a crow arrives (first PIR trigger of a visit).
// Check STAGE_CONFIGS[config.stage].gate_opens_on_pir — if true, call gate_open().
// For stage 1: servo_doser_dispense(LVT hopper) if reward chamber is empty.
// Log the event.
static void handle_pir_tripped(crowbox_event_msg_t *msg) {
    ESP_LOGI(TAG, "PIR tripped");
    s_crow_departed = false;
    switch (s_current_state) {
        case STATE_IDLE:
            s_visit_start_ms = msg->timestamp_ms;
            log_visit_start(g_config.stage);
            if (STAGE_CONFIGS[g_config.stage].gate_opens_on_pir) {
            gate_open();
            }
            transition_to(STATE_PIR_TRIPPED);
            break;
        case STATE_ERROR:
            vTaskDelay(pdMS_TO_TICKS(5000));
            esp_restart();
            break;
        default: break;
    }
    


}

// handle_pir_cleared
// Called when crow departs (no PIR trigger for DEPARTURE_DEBOUNCE_MS).
// If gate is open: gate_close().
// If reward chamber is empty (weight_read_grams(WEIGHT_SENSOR_REWARD) < threshold):
//   servo_doser_dispense to restock LVT.
// If deposit_pending_clear flag is set: transition to trapdoor clearing sequence.
// Otherwise: sleep_enter_deep().

/*
if cooldown: close gate, trapdoor sequence, if reward chamber empty, roll for LVT
*/

static void handle_pir_cleared(crowbox_event_msg_t *msg) {
    ESP_LOGI(TAG, "PIR cleared — crow departed");
    s_crow_departed = true;
    switch(s_current_state){
        case STATE_PIR_TRIPPED:
        transition_to(STATE_COOLDOWN);
        //fallthrough
        case STATE_COOLDOWN:
        run_cooldown_sequence();
        break;
        default: break;
    }
}

// handle_deposit_weight_detected
// Weight above baseline detected in deposit chamber.
// Start a settle timer — wait for weight to stabilize before validating.
static void handle_deposit_weight_detected(crowbox_event_msg_t *msg) {
    ESP_LOGI(TAG, "deposit weight detected: %ld g", msg->value);
    switch(s_current_state) {
        case STATE_PIR_TRIPPED: 
            transition_to(STATE_DEPOSIT_DETECTED); 
            break;
        case STATE_COOLDOWN:
            ESP_LOGW(TAG, "deposit weight detected during cooldown state");
            break;
        default: break;
    }
}

// handle_deposit_weight_settled
// Weight has been stable. Run classify_object() (stub: returns true for any weight > threshold).
// If valid: post EVENT_OBJECT_VALID.
// If invalid: post EVENT_OBJECT_INVALID.
static void handle_deposit_weight_settled(crowbox_event_msg_t *msg) {
    ESP_LOGI(TAG, "deposit weight settled");
    switch (s_current_state) {
        case STATE_DEPOSIT_DETECTED:
        s_last_deposit_weight_g = msg->value;
        transition_to(STATE_VALIDATING);
        if (classify_object(s_last_deposit_weight_g)) {
            crowbox_event_msg_t validated_msg = 
            {
                .type = EVENT_OBJECT_VALID,
                .value = 1,
                .timestamp_ms = (esp_timer_get_time()/1000)
            };
            event_bus_post(validated_msg);
        }
        else {
            crowbox_event_msg_t invalidated_msg = 
            {
                .type = EVENT_OBJECT_INVALID,
                .value = 1,
                .timestamp_ms = (esp_timer_get_time()/1000)
            };
            event_bus_post(invalidated_msg);
        }
        transition_to(STATE_DISPENSING);
        break;
        default: break;
    }
}

// handle_object_valid
// Roll reward schedule (check STAGE_CONFIGS[stage].hvt_chance_pct against rand()).
// If reward: roll jackpot (jackpot_chance_pct). Dispatch servo_doser_dispense calls
//   based on reward tier (VOD = hopper 2, JACKPOT = hoppers 1+2+3).
// Play tone (buzzer_play_accept or buzzer_play_jackpot).
// Set LED (green for accept, blue for jackpot via mcp23017_set_pin).
// Set deposit_pending_clear = true.
static void handle_object_valid(crowbox_event_msg_t *msg) {
    ESP_LOGI(TAG, "object valid");
    s_object_in_trapdoor = true;
    s_last_deposit_valid = true;
    log_deposit(s_last_deposit_weight_g, true);

    config_on_deposit_success();

    if (s_current_state == STATE_DISPENSING) {
        bool hvt_hit = (rand() % 100) < STAGE_CONFIGS[g_config.stage].hvt_chance_pct;
        if (hvt_hit) {
            bool jackpot_hit = (rand() % 100) < STAGE_CONFIGS[g_config.stage].jackpot_chance_pct;
            if (jackpot_hit) {
                servo_doser_dispense(0);
                servo_doser_dispense(1);
                servo_doser_dispense(2);
                s_reward_in_chamber = true;
                buzzer_play_jackpot();
                mcp23017_set_pin(0, MCP_BIT_LED_BLUE, true);
                log_reward(REWARD_TYPE_EHVT);
                if (!gate_is_open()) {
                    gate_open();
                } else {
                    ESP_LOGI(TAG, "bug: gate open during dispensing");
                }
            } else {
                servo_doser_dispense(1);
                s_reward_in_chamber = true;
                buzzer_play_accept();
                mcp23017_set_pin(0, MCP_BIT_LED_GREEN, true);
                log_reward(REWARD_TYPE_HVT);
            }
        } else {
            buzzer_play_accept();
            mcp23017_set_pin(0, MCP_BIT_LED_GREEN, true);
        }
    }
    transition_to(STATE_COOLDOWN);
    if (s_crow_departed) {
        run_cooldown_sequence();
    }
}

// handle_object_invalid
// Play reject tone (buzzer_play_reject).
// Set red LED (mcp23017_set_pin port A, MCP_BIT_LED_RED, true).
// Set deposit_pending_clear = true — chamber still needs clearing even on rejection.
static void handle_object_invalid(crowbox_event_msg_t *msg) {
    ESP_LOGI(TAG, "object invalid");
    s_object_in_trapdoor = true;
    s_last_deposit_valid = false;
    log_deposit(s_last_deposit_weight_g, false);
    if (s_current_state == STATE_DISPENSING) {
        mcp23017_set_pin(0, MCP_BIT_LED_RED, true);
        buzzer_play_reject();
    }
    transition_to(STATE_COOLDOWN);
    if (s_crow_departed) {
        run_cooldown_sequence();
    }
}

// state_machine_task
// Main event loop — never returns.
// Blocks on event_bus_receive with 1-second timeout.
// On event received: switch on msg.type, call the appropriate handler above.
// On timeout (1 second, no event): call pir_check_departure().
static void state_machine_task(void *arg) {
    crowbox_event_msg_t msg;
    while (1) {
        if (event_bus_receive(&msg, pdMS_TO_TICKS(1000))) {
            switch (msg.type) {
                case EVENT_PIR_TRIPPED:                handle_pir_tripped(&msg);             break;
                case EVENT_PIR_CLEARED:                handle_pir_cleared(&msg);             break;
                case EVENT_DEPOSIT_WEIGHT_DETECTED:    handle_deposit_weight_detected(&msg); break;
                case EVENT_DEPOSIT_WEIGHT_SETTLED:     handle_deposit_weight_settled(&msg);  break;
                case EVENT_OBJECT_VALID:               handle_object_valid(&msg);            break;
                case EVENT_OBJECT_INVALID:             handle_object_invalid(&msg);          break;
                default: break;
            }
        } else {
            pir_check_departure();
        }
    }
}

// state_machine_start
// Creates the state_machine_task FreeRTOS task.
// Stack size 4096, priority 5 — adjust if stack overflows appear in logs.
void state_machine_start(void) {
    xTaskCreate(state_machine_task, "state_machine", 4096, NULL, 5, NULL);
}
