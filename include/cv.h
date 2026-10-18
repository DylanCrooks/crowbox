#pragma once
#include <stdbool.h>
#include <stdint.h>

// Returns true if the object in the deposit chamber is valid.
// Phase 6+: sends a UART request to the vision board (ESP32-P4) and waits for its classification result.
// Until then: stub returns true (accept everything above the weight threshold).
bool classify_object(int32_t weight);
