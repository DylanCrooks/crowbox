#include "i2c_bus.h"
#include "pinout.h"

void notecard_init(void){
    i2c_master_bus_handle_t bus_handle = i2c_bus_get_handle();
    i2c_device_config_t notecard_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_ADDR_NOTECARD,
        .scl_speed_hz = 100000 //mcp can go higher, esp32 can go higher, but notecard can go max 100kHz
    };
    i2c_master_dev_handle_t notecard_handle;
    i2c_master_bus_add_device(bus_handle, &notecard_config, &notecard_handle);
};