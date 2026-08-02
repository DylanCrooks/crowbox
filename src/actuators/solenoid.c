#include "solenoid.h"
#include "mcp23017.h"
#include "pinout.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SOLENOID_UNLOCK_TICKS pdMS_TO_TICKS(50)

static bool s_locked = true;
bool solenoid_is_locked(void) { return s_locked; }
// solenoid_unlock
// Call mcp23017_set_pin(1, MCP_BIT_SOLENOID, true).
// Port B (1), bit MCP_BIT_SOLENOID (4 per pinout.h). Setting high energizes the coil.
// Add a short vTaskDelay after — solenoid needs a few ms to physically retract.
void solenoid_unlock(void) {
    mcp23017_set_pin(1,MCP_BIT_SOLENOID,true);
    vTaskDelay(SOLENOID_UNLOCK_TICKS);
    s_locked = false;
}

void solenoid_lock(void) {
    mcp23017_set_pin(1, MCP_BIT_SOLENOID, false);
    vTaskDelay(pdMS_TO_TICKS(50));
    s_locked = true;
}
