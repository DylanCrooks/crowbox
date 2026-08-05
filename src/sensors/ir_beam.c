#include "ir_beam.h"
#include "mcp23017.h"

// Beam HIGH = unbroken = nothing in hopper = empty.
// Beam LOW  = broken   = seeds blocking beam = hopper has food.
bool ir_beam_hopper_is_empty(uint8_t hopper_id) {
    uint8_t gpa = mcp23017_read_port(0);
    return (gpa >> hopper_id) & 1;
}
