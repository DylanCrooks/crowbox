#include "gate.h"
#include "mcp23017.h"
#include "pinout.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// CLAUDE TODO: tune this on hardware — time for actuator to fully extend/retract
#define GATE_TRAVEL_TICKS pdMS_TO_TICKS(2000)

// gate_open
// mcp23017_set_pin calls for actuator A: IN1 high, IN2 low (port B, bits 0 and 1)
// mcp23017_set_pin calls for actuator B: IN1 high, IN2 low (port B, bits 2 and 3)
// vTaskDelay(GATE_TRAVEL_TICKS), then gate_stop()
void gate_open(void) {}

// gate_close
// Same as gate_open but IN1 low, IN2 high for both actuators (reverse direction)
// vTaskDelay(GATE_TRAVEL_TICKS), then gate_stop()
void gate_close(void) {}

// gate_stop
// Set all 4 direction pins low via mcp23017_set_pin:
// port B bits 0, 1, 2, 3 all false
void gate_stop(void) {}
