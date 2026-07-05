#include "solenoid.h"
#include "mcp23017.h"
#include "pinout.h"

// solenoid_unlock
// Call mcp23017_set_pin(1, MCP_BIT_SOLENOID, true).
// Port B (1), bit MCP_BIT_SOLENOID (4 per pinout.h). Setting high energizes the coil.
// Add a short vTaskDelay after — solenoid needs a few ms to physically retract.
void solenoid_unlock(void) {
    // CLAUDE TODO: mcp23017_set_pin(1, MCP_BIT_SOLENOID, true)
    // CLAUDE TODO: vTaskDelay(pdMS_TO_TICKS(50)) — give bolt ~50ms to physically retract before servo moves
}

// solenoid_lock
// Call mcp23017_set_pin(1, MCP_BIT_SOLENOID, false).
// De-energizes coil, spring re-engages bolt.
// Add a short vTaskDelay after — spring needs time to seat fully before servo moves.
void solenoid_lock(void) {
    // CLAUDE TODO: mcp23017_set_pin(1, MCP_BIT_SOLENOID, false)
    // CLAUDE TODO: vTaskDelay(pdMS_TO_TICKS(50)) — give spring ~50ms to fully seat before load is applied
}
