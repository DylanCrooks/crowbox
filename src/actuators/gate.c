#include "gate.h"
#include "mcp23017.h"
#include "pinout.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define GATE_TRAVEL_TICKS pdMS_TO_TICKS(10000)
// Longer than actual travel — guarantees full retract regardless of starting position.
#define GATE_HOME_TICKS pdMS_TO_TICKS(12000)

static bool s_gate_open = false;
bool gate_is_open(void) { return s_gate_open; }

// Drive both actuators in the retract direction long enough to guarantee full retract
// from any starting position. Call once on startup before entering the main loop.
void gate_home(void) {
    mcp23017_set_pin(1, MCP_BIT_ACT_A_IN1, false);
    mcp23017_set_pin(1, MCP_BIT_ACT_A_IN2, true);
    mcp23017_set_pin(1, MCP_BIT_ACT_B_IN1, false);
    mcp23017_set_pin(1, MCP_BIT_ACT_B_IN2, true);
    vTaskDelay(GATE_HOME_TICKS);
    gate_stop();
    s_gate_open = false;
}

// gate_open
// mcp23017_set_pin calls for actuator A: IN1 high, IN2 low (port B, bits 0 and 1)
// mcp23017_set_pin calls for actuator B: IN1 high, IN2 low (port B, bits 2 and 3)
// vTaskDelay(GATE_TRAVEL_TICKS), then gate_stop()
void gate_open(void) {
    mcp23017_set_pin(1,MCP_BIT_ACT_A_IN1, true);
    mcp23017_set_pin(1,MCP_BIT_ACT_A_IN2, false);
    mcp23017_set_pin(1,MCP_BIT_ACT_B_IN1, true);
    mcp23017_set_pin(1,MCP_BIT_ACT_B_IN2, false);
    vTaskDelay(GATE_TRAVEL_TICKS);
    gate_stop();
    s_gate_open = true;
}

// gate_close
// Same as gate_open but IN1 low, IN2 high for both actuators (reverse direction)
// vTaskDelay(GATE_TRAVEL_TICKS), then gate_stop()
void gate_close(void) {
    // CLAUDE TODO: same as gate_open but IN1 and IN2 swapped — reversed polarity = reversed direction
    mcp23017_set_pin(1, MCP_BIT_ACT_A_IN1, false);
    mcp23017_set_pin(1, MCP_BIT_ACT_A_IN2, true);
    mcp23017_set_pin(1, MCP_BIT_ACT_B_IN1, false);
    mcp23017_set_pin(1, MCP_BIT_ACT_B_IN2, true);
    vTaskDelay(GATE_TRAVEL_TICKS);
    gate_stop();
    s_gate_open = false;
}

// gate_stop
// Set all 4 direction pins low via mcp23017_set_pin:
// port B bits 0, 1, 2, 3 all false
void gate_stop(void) {
    // CLAUDE TODO: set all 4 direction pins false — both actuators coast to stop
    mcp23017_set_pin(1, MCP_BIT_ACT_A_IN1, false);
    mcp23017_set_pin(1, MCP_BIT_ACT_A_IN2, false);
    mcp23017_set_pin(1, MCP_BIT_ACT_B_IN1, false);
    mcp23017_set_pin(1, MCP_BIT_ACT_B_IN2, false);
}
