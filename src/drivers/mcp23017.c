#include "mcp23017.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"

// Shadow registers — local copies of what was last written to the chip.
// Rule: never read-modify-write the MCP23017 over I2C.
// Instead: update shadow with |= or &=~, then write the full shadow byte.
static uint8_t s_shadow_a = 0x00;
static uint8_t s_shadow_b = 0x00;

// mcp23017_init
// Step 1: configure i2c_config_t struct (mode, sda_io_num, scl_io_num, clk_speed = 400000)
// Step 2: call i2c_param_config(I2C_NUM_0, &conf)
// Step 3: call i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0)
// Step 4: write MCP_REG_IODIRA (0x00) with value 0x0F  — lower nibble inputs, upper nibble outputs
// Step 5: write MCP_REG_IODIRB (0x01) with value 0xE0  — bits 0-4 outputs, bits 5-7 inputs
void mcp23017_init(void) {}

// mcp23017_set_pin
// If high:  shadow |=  (1 << bit)
// If low:   shadow &= ~(1 << bit)
// Determine which shadow (A or B) from port argument.
// Write updated shadow to MCP_REG_OLATA or MCP_REG_OLATB via I2C.
// I2C write sequence: start → device address (I2C_ADDR_MCP23017 << 1 | write) → register → data → stop
void mcp23017_set_pin(uint8_t port, uint8_t bit, bool high) {}

// mcp23017_read_porta
// I2C write the register address MCP_REG_GPIOA (0x12), then I2C read 1 byte back.
// Return that byte. Caller checks individual bits with (result >> bit) & 1.
uint8_t mcp23017_read_porta(void) { return 0; }
