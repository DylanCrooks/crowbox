#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "logging.h"
#include "i2c_bus.h"
#include "mcp23017.h"
#include "gate.h"
#include "weight.h"
#include "pir.h"
#include "servo.h"
#include "solenoid.h"
#include "buzzer.h"
#include "event_bus.h"
#include "state_machine.h"
#include "tests.h"
#include "esp_sleep.h"
#include "esp_random.h"
#include "nvs_flash.h"
#include "config.h"
#include "vision_link.h"


static const char *TAG = "crowbox";

void app_main(void) {
    ESP_LOGI(TAG, "CrowBox starting");
    nvs_flash_init();
    config_init();
    i2c_bus_init();
    mcp23017_init();
    gate_home();
    weight_init(WEIGHT_SENSOR_DEPOSIT);
    weight_init(WEIGHT_SENSOR_REWARD);
    pir_init();
    servo_init();
    buzzer_init();
    vision_link_init();
    event_bus_init();
    state_machine_start();
    srand(esp_random());
    

    log_boot(g_config.stage, g_config.successful_deposit_count);
    
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
        pir_post_arrival_event();
    }
    // Swap which test to run — comment all out for production
    //test_solenoid_and_servo();
    test_gate();
    // test_weight_sensors();
    // test_mcp23017_leds();
}
