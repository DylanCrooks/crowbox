#pragma once

// solenoid_unlock
// Energize the solenoid (set MCP_BIT_SOLENOID high via mcp23017_set_pin).
// The deadbolt is spring-loaded locked — power retracts it (unlocks).
void solenoid_unlock(void);

// solenoid_lock
// De-energize the solenoid (set MCP_BIT_SOLENOID low).
// Spring pushes bolt back into locked position. Always call this after trapdoor closes.
void solenoid_lock(void);
