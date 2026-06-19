#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "crowbox";

void app_main(void) {
    ESP_LOGI(TAG, "CrowBox firmware starting");

    // Initialize modules (implementations come later)
    // weight_init();
    // ir_sensor_init();
    // servo_init();
    // buzzer_init();
    // event_bus_init();
    // state_machine_start();

    // Keep app_main alive - FreeRTOS tasks run independently
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}