#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "crowbox.h"
#include "stdint.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "weight.h"
#include "pir.h"
#include "solenoid.h"
#include "gate.h"
#include "servo.h"
#include "buzzer.h"
#include "i2c_bus.h"
#include "mcp23017.h"
#include "sleep.h"

static const char *TAG = "crowbox";


void app_main(void) {
    ESP_LOGI(TAG, "CrowBox firmware starting");
    
    // Initialize modules (implementations come later)
    i2c_bus_init();
    mcp23017_init();
    weight_init(WEIGHT_SENSOR_DEPOSIT);
    weight_init(WEIGHT_SENSOR_REWARD);
    pir_init();
    servo_init();
    buzzer_init();
    event_bus_init();
    state_machine_start();

    while(1){
    mcp23017_set_pin(0,4,true);
    mcp23017_set_pin(0,5,true);
    mcp23017_set_pin(0,6,true);
    vTaskDelay(pdMS_TO_TICKS(500));
    mcp23017_set_pin(0,4,false);
    mcp23017_set_pin(0,5,false);
    mcp23017_set_pin(0,6,false);
    vTaskDelay(pdMS_TO_TICKS(500));
    uint8_t read_port = mcp23017_read_port(0);
    ESP_LOGI("mcp_port_a","port A: %02X", read_port);

    }
}