#pragma once

// state_machine_start
// Creates the state machine FreeRTOS task (xTaskCreate).
// The task loops forever: blocks on event_bus_receive with a 1-second timeout.
// On event: dispatches to the appropriate static handler via switch(msg.type).
// On timeout (no event for 1 second): calls pir_check_departure().
// Call this once from app_main after all drivers are initialized.
void state_machine_start(void);
