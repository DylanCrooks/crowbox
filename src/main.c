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

static const char *TAG = "crowbox";

void app_main(void) {
    ESP_LOGI(TAG, "CrowBox starting — solenoid + MG996R servo test");

    i2c_bus_init();
    mcp23017_init();
    weight_init(WEIGHT_SENSOR_DEPOSIT);
    weight_init(WEIGHT_SENSOR_REWARD);
    pir_init();
    servo_init();
    buzzer_init();
    event_bus_init();
    state_machine_start();

    int cycle = 0;
    while (1) {
        // Solenoid: unlock every 10s, hold 500ms (long enough to retract, short enough
        // to avoid heat buildup), then lock and wait for flyback to dissipate before next cycle
        if (cycle % 10 == 0) {
            ESP_LOGI("solenoid", "unlocking");
            solenoid_unlock();               // sets GPB4 high, already has 50ms delay inside
            vTaskDelay(pdMS_TO_TICKS(450));  // hold energized for 500ms total (50 inside + 450 here)
            ESP_LOGI("solenoid", "locking");
            solenoid_lock();                 // sets GPB4 low, already has 50ms delay inside
            vTaskDelay(pdMS_TO_TICKS(200));  // let flyback dissipate before next cycle
        }

        // Servo: sweep 0->90->0 every 5s
        if (cycle % 5 == 0) {
            ESP_LOGI("servo", "trapdoor -> 0 deg");
            servo_trapdoor_set_angle(0);
            vTaskDelay(pdMS_TO_TICKS(1000));
            ESP_LOGI("servo", "trapdoor -> 90 deg");
            servo_trapdoor_set_angle(90);
            vTaskDelay(pdMS_TO_TICKS(1000));
            ESP_LOGI("servo", "trapdoor -> 0 deg");
            servo_trapdoor_set_angle(0);
        }

        cycle++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
