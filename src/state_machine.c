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


#define TAG "state_machine"

static crowbox_state_t s_current_state = STATE_IDLE;
static bool s_crow_departed = false;
static bool s_object_in_chamber = false;
static bool s_deposit_logged = false;
static int64_t s_visit_start_ms = 0;


static const char *state_name(crowbox_state_t s) {
    switch (s) {
        case STATE_IDLE:                return "IDLE";
        case STATE_PIR_TRIPPED:         return "PIR_TRIPPED";
        case STATE_DEPOSIT_DETECTED:    return "DEPOSIT_DETECTED";
        case STATE_VALIDATING:          return "VALIDATING";
        case STATE_DISPENSING:          return "DISPENSING";
        case STATE_TRAPDOOR_UNLOCKING:  return "TRAPDOOR_UNLOCKING";
        case STATE_TRAPDOOR_OPENING:    return "TRAPDOOR_OPENING";
        case STATE_TRAPDOOR_CLOSING:    return "TRAPDOOR_CLOSING";
        case STATE_TRAPDOOR_LOCKING:    return "TRAPDOOR_LOCKING";
        case STATE_COOLDOWN:            return "COOLDOWN";
        case STATE_LOGGING:             return "LOGGING";
        case STATE_ERROR:               return "ERROR";
        default:                        return "UNKNOWN";
    }
}

static void transition_to(crowbox_state_t new_state) {
    ESP_LOGI(TAG, "state: %s -> %s", state_name(s_current_state), state_name(new_state));
    s_current_state = new_state;
}

// handle_pir_tripped
// Called when a crow arrives (first PIR trigger of a visit).
// Check STAGE_CONFIGS[config.stage].gate_opens_on_pir — if true, call gate_open().
// For stage 1: servo_doser_dispense(LVT hopper) if reward chamber is empty.
// Log the event.
static void handle_pir_tripped(crowbox_event_msg_t *msg) {
    ESP_LOGI(TAG, "PIR tripped");
    
    switch (s_current_state) {
        case STATE_IDLE:
            if (STAGE_CONFIGS[g_config.stage].gate_opens_on_pir) {
            gate_open();
            }                
            transition_to(STATE_PIR_TRIPPED);
            break;
        case STATE_ERROR:
            vTaskDelay(pdMS_TO_TICKS(5000));
            esp_restart();
            break;
    }
    


}

// handle_pir_cleared
// Called when crow departs (no PIR trigger for DEPARTURE_DEBOUNCE_MS).
// If gate is open: gate_close().
// If reward chamber is empty (weight_read_grams(WEIGHT_SENSOR_REWARD) < threshold):
//   servo_doser_dispense to restock LVT.
// If deposit_pending_clear flag is set: transition to trapdoor clearing sequence.
// Otherwise: sleep_enter_deep().
static void handle_pir_cleared(crowbox_event_msg_t *msg) {
    ESP_LOGI(TAG, "PIR cleared — crow departed");
    s_crow_departed = true;
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
            //transition_to(STATE_TRAPDOOR_UNLOCKING) ??
            break;
    }
}

// handle_deposit_weight_settled
// Weight has been stable. Run classify_object() (stub: returns true for any weight > threshold).
// If valid: post EVENT_OBJECT_VALID.
// If invalid: post EVENT_OBJECT_INVALID.
static void handle_deposit_weight_settled(crowbox_event_msg_t *msg) {
    ESP_LOGI(TAG, "deposit weight settled");
    switch (s_current_state) {
        case STATE_VALIDATING:
        classify_object();
        transition_to(STATE_DISPENSING);
        break;
    
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
}

// handle_object_invalid
// Play reject tone (buzzer_play_reject).
// Set red LED (mcp23017_set_pin port A, MCP_BIT_LED_RED, true).
// Set deposit_pending_clear = true — chamber still needs clearing even on rejection.
static void handle_object_invalid(crowbox_event_msg_t *msg) {
    ESP_LOGI(TAG, "object invalid");
}

// handle_reward_tray_empty
// Reward chamber weight dropped back to baseline — crow ate the food.
// Close gate (gate_close) if not already closed.
// Restock LVT (servo_doser_dispense hopper 0) so next visit has a preset staged.
static void handle_reward_tray_empty(crowbox_event_msg_t *msg) {
    ESP_LOGI(TAG, "reward tray empty");
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
                case EVENT_REWARD_TRAY_EMPTY:          handle_reward_tray_empty(&msg);       break;
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
