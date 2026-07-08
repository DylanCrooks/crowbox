#include "mcp23017.h"
#include "i2c_bus.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"

// Shadow registers — local copies of what was last written to the chip.
// Rule: never read-modify-write the MCP23017 over I2C.
// Instead: update shadow with |= or &=~, then write the full shadow byte.
static uint8_t s_shadow_a = 0x00;
static uint8_t s_shadow_b = 0x00;

static i2c_master_dev_handle_t s_mcp_handle;

void mcp23017_init(void) {
    i2c_master_bus_handle_t bus_handle = i2c_bus_get_handle();
    const i2c_device_config_t mcp_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x20,
        .scl_speed_hz = 100000 //mcp can go higher, esp32 can go higher, but notecard can go max 100kHz
    };
    i2c_master_bus_add_device(bus_handle, &mcp_config, &s_mcp_handle);
    const uint8_t iodira_buf[2] = {MCP_REG_IODIRA, 0x0F}; // set gpioa0-3 as input, rest as outputs
    
    i2c_master_transmit(s_mcp_handle, iodira_buf, 2, pdMS_TO_TICKS(100)); 
    const uint8_t iodirb_buf[2] = {MCP_REG_IODIRB, 0xE0}; //set 5,6,7 as input, rest outputs
    i2c_master_transmit(s_mcp_handle, iodirb_buf, 2, pdMS_TO_TICKS(100));
}

// mcp23017_set_pin
// If high:  shadow |=  (1 << bit)
// If low:   shadow &= ~(1 << bit)
//shadow keeps track of output pin state so we don't write 0 to output accidentally.
void mcp23017_set_pin(bool port , uint8_t bit, bool high) {
    if (!port){
        if (high) {
            s_shadow_a |= (1 << bit);
        }
        else if (!high) {
            s_shadow_a &= ~(1 << bit);
        }
        const uint8_t gpioa_buf[2] = {MCP_REG_GPIOA, s_shadow_a};
        i2c_master_transmit(s_mcp_handle, gpioa_buf, 2, pdMS_TO_TICKS(100));
    }
    else if (port) {
        if (high) {
            s_shadow_b |= (1 << bit);
        }
        else if (!high){
            s_shadow_b &= ~(1 << bit);
        }
        const uint8_t gpiob_buf[2] = {MCP_REG_GPIOB, s_shadow_b};
        i2c_master_transmit(s_mcp_handle, gpiob_buf, 2, pdMS_TO_TICKS (100));

    };
}

// mcp23017_read_porta
// I2C write the register address MCP_REG_GPIOA (0x12), then I2C read 1 byte back.
// Return that byte. Caller checks individual bits with (result >> bit) & 1.
uint8_t mcp23017_read_port(bool port) {
    uint8_t result;
    uint8_t reg;
    if (!port) {
        reg = MCP_REG_GPIOA;
        if ((i2c_master_transmit_receive(s_mcp_handle, &reg, 1, &result, 1, pdMS_TO_TICKS(100))) == ESP_OK) {
            return result;
        };
    }
    else if (port) {
        reg = MCP_REG_GPIOB;
        if ((i2c_master_transmit_receive(s_mcp_handle, &reg, 1, &result, 1, pdMS_TO_TICKS(100))) == ESP_OK) {
            return result;
        }
    }
    return 0;
}
