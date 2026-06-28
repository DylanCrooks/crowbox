#include "crowbox.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "esp_log.h"
#include "crowbox.h"
#include "stdint.h"
#include "driver/gpio.h"

gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_27),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    /*
    gpio_config(&io_conf);
    while (1) {
        int level = gpio_get_level(GPIO_NUM_27);
        ESP_LOGI(TAG, "GPIO27 level: %d", level);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
        */