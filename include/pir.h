#pragma once

// pir_init
// Configure GPIO34 as input, rising edge interrupt. Install ISR service and handler.
void pir_init(void);

// pir_check_departure
// Call ~1Hz from the state machine's timeout path.
// If s_crow_present and time since last trigger > DEPARTURE_DEBOUNCE_MS: post EVENT_PIR_CLEARED.
void pir_check_departure(void);
