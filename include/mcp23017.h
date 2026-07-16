#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "pinout.h"

// mcp23017_init
// Initialize I2C master on PIN_I2C_SDA / PIN_I2C_SCL at 400kHz.
// Write IODIRA: lower 4 bits inputs (IR beams GPA0-3), upper 4 bits outputs (LEDs, COB).
// Write IODIRB: lower 5 bits outputs (gate actuators, solenoid), upper 3 inputs.
void mcp23017_init(void);

// mcp23017_set_pin
// Set a single expander pin high or low using the shadow register pattern.
// port: 0 = port A (GPIOA), 1 = port B (GPIOB).
// bit: 0-7 — use MCP_BIT_* constants from pinout.h.
// Updates the internal shadow byte then writes the full byte to the chip via I2C.
void mcp23017_set_pin(bool port, uint8_t bit, bool high);

// mcp23017_read_porta
// Read the current logic levels on port A or B (GPIOA register 0x12).
uint8_t mcp23017_read_port(bool port);
