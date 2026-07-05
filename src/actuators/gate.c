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
void gate_open(void) {
    // CLAUDE TODO: mcp23017_set_pin(1, MCP_BIT_GATE_A_IN1, true)   — port B = 1, see pinout.h for bit nums
    // CLAUDE TODO: mcp23017_set_pin(1, MCP_BIT_GATE_A_IN2, false)
    // CLAUDE TODO: mcp23017_set_pin(1, MCP_BIT_GATE_B_IN1, true)
    // CLAUDE TODO: mcp23017_set_pin(1, MCP_BIT_GATE_B_IN2, false)
    // CLAUDE TODO: vTaskDelay(GATE_TRAVEL_TICKS)
    // CLAUDE TODO: gate_stop()
}

// gate_close
// Same as gate_open but IN1 low, IN2 high for both actuators (reverse direction)
// vTaskDelay(GATE_TRAVEL_TICKS), then gate_stop()
void gate_close(void) {
    // CLAUDE TODO: same as gate_open but IN1 and IN2 swapped — reversed polarity = reversed direction
    // CLAUDE TODO: mcp23017_set_pin(1, MCP_BIT_GATE_A_IN1, false)
    // CLAUDE TODO: mcp23017_set_pin(1, MCP_BIT_GATE_A_IN2, true)
    // CLAUDE TODO: mcp23017_set_pin(1, MCP_BIT_GATE_B_IN1, false)
    // CLAUDE TODO: mcp23017_set_pin(1, MCP_BIT_GATE_B_IN2, true)
    // CLAUDE TODO: vTaskDelay(GATE_TRAVEL_TICKS)
    // CLAUDE TODO: gate_stop()
}

// gate_stop
// Set all 4 direction pins low via mcp23017_set_pin:
// port B bits 0, 1, 2, 3 all false
void gate_stop(void) {
    // CLAUDE TODO: set all 4 direction pins false — both actuators coast to stop
    // CLAUDE TODO: mcp23017_set_pin(1, MCP_BIT_GATE_A_IN1, false)
    // CLAUDE TODO: mcp23017_set_pin(1, MCP_BIT_GATE_A_IN2, false)
    // CLAUDE TODO: mcp23017_set_pin(1, MCP_BIT_GATE_B_IN1, false)
    // CLAUDE TODO: mcp23017_set_pin(1, MCP_BIT_GATE_B_IN2, false)
}
