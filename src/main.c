#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "crowbox.h"
#include "stdint.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

static const char *TAG = "crowbox";

static uint32_t angle_to_duty(int angle) {
        uint32_t us = 500 + (angle * (2400 - 500) / 180);
        return (us * 8191) / 20000;
    };

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
    ledc_timer_config_t timer = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .duty_resolution = LEDC_TIMER_13_BIT,
    .timer_num = LEDC_TIMER_0,
    .freq_hz = 50,
    .clk_cfg = LEDC_AUTO_CLK
};
ledc_timer_config(&timer);

ledc_channel_config_t channel = {
    .gpio_num = GPIO_NUM_16,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .timer_sel = LEDC_TIMER_0,
    .duty = angle_to_duty(0),
    .hpoint = 0
};
ledc_channel_config(&channel);
    while (1) {
        for (int angle = 0; angle <= 180; angle += 10) {
            uint32_t duty = angle_to_duty(angle);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            ESP_LOGI(TAG, "Set servo to angle: %d degrees", angle);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        for (int angle = 180; angle >= 0; angle -= 10) {
            uint32_t duty = angle_to_duty(angle);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            ESP_LOGI(TAG, "Set servo to angle: %d degrees", angle);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
}