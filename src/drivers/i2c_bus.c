#include "i2c_bus.h"



i2c_master_bus_handle_t i2c_bus_get_handle(){
    i2c_master_bus_handle_t bus_handle;
    return bus_handle;
};
void i2c_bus_init(){
    i2c_master_bus_handle_t bus_handle = i2c_bus_get_handle();
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_21,
        .scl_io_num = GPIO_NUM_22,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        //if logic analyser sees glitches, increase glitch ignore cnt
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_new_master_bus(&bus_handle,&bus_config);
};
