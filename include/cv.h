#pragma once
#include <stdbool.h>
#include <stdint.h>

// Returns true if the object in the deposit chamber is valid.
// Below CV_STRICTNESS_LEVEL: weight alone decides (the vision board is still asked,
// without waiting, so it saves a photo). At or above it: the vision board's result
// decides, falling back to weight alone on timeout or error.
bool classify_object(int32_t weight);

// Chamber illumination (COB LED strip). The camera needs it on for every photo; it is
// on for the whole visit and off before deep sleep (MCP23017 outputs survive sleep).
void cv_light(bool on);

// Pit is empty (trapdoor just closed): has the vision board photograph it as the
// background for the next deposit. Blocks until it replies or times out.
void cv_capture_reference(void);
