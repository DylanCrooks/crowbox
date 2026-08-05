#pragma once
#include <stdbool.h>
#include <stdint.h>

// Returns true if the hopper is empty (beam unbroken = nothing blocking it).
// hopper_id: 0–3 matching GPA0–GPA3 on the MCP23017.
bool ir_beam_hopper_is_empty(uint8_t hopper_id);
