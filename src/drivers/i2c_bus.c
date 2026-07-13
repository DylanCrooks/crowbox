#include "i2c_bus.h"
#include "pinout.h"


static i2c_master_bus_handle_t s_bus_handle;

i2c_master_bus_handle_t i2c_bus_get_handle(){
    
    return s_bus_handle;
};
void i2c_bus_init(){
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        //if logic analyser sees glitches, increase glitch ignore cnt
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_new_master_bus(&bus_config,&s_bus_handle);
};
