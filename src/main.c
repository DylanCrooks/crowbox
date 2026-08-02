#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2c_bus.h"
#include "mcp23017.h"
#include "weight.h"
#include "pir.h"
#include "servo.h"
#include "solenoid.h"
#include "buzzer.h"
#include "event_bus.h"
#include "state_machine.h"
#include "tests.h"

static const char *TAG = "crowbox";

void app_main(void) {
    ESP_LOGI(TAG, "CrowBox starting");

    i2c_bus_init();
    mcp23017_init();
    weight_init(WEIGHT_SENSOR_DEPOSIT);
    weight_init(WEIGHT_SENSOR_REWARD);
    pir_init();
    servo_init();
    buzzer_init();
    event_bus_init();
    state_machine_start();

    // Swap which test to run — comment all out for production
    //test_solenoid_and_servo();
    test_gate();
    // test_weight_sensors();
    // test_mcp23017_leds();
}
