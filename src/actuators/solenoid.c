#include "solenoid.h"
#include "mcp23017.h"
#include "pinout.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SOLENOID_UNLOCK_TICKS pdMS_TO_TICKS(50)
// solenoid_unlock
// Call mcp23017_set_pin(1, MCP_BIT_SOLENOID, true).
// Port B (1), bit MCP_BIT_SOLENOID (4 per pinout.h). Setting high energizes the coil.
// Add a short vTaskDelay after — solenoid needs a few ms to physically retract.
void solenoid_unlock(void) {
    mcp23017_set_pin(1,MCP_BIT_SOLENOID,true);
    vTaskDelay(SOLENOID_UNLOCK_TICKS);
}

// solenoid_lock
// Call mcp23017_set_pin(1, MCP_BIT_SOLENOID, false).
// De-energizes coil, spring re-engages bolt.
// Add a short vTaskDelay after — spring needs time to seat fully before servo moves.
void solenoid_lock(void) {
    mcp23017_set_pin(1, MCP_BIT_SOLENOID, false);
    vTaskDelay(pdMS_TO_TICKS(50));
}
