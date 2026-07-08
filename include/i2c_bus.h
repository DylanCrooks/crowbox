#include "driver/i2c_master.h"
#include "driver/i2c_slave.h"
#include "freertos/FreeRTOS.h"

void i2c_bus_init();
i2c_master_bus_handle_t i2c_bus_get_handle();