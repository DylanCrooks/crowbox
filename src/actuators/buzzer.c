#include<stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "crowbox.h"
#include "stdint.h"
#include "driver/gpio.h"
#include "driver/ledc.h"


void buzzer_play_reject(void) {ESP_LOGI("buzzer", "playing reject sound");
        ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, 200);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 128);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        vTaskDelay(pdMS_TO_TICKS(300));
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void buzzer_play_accept(void) {
        ESP_LOGI("buzzer", "playing tone 2");
        ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, 2000);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 128);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        vTaskDelay(pdMS_TO_TICKS(200));
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void buzzer_play_jackpot(void) {
        ESP_LOGI("buzzer", "playing tone 3");
        uint32_t freqs[] = {400, 600, 800, 1000, 1400, 2000, 2800};
        for (int i = 0; i < 7; i++) {
            ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freqs[i]);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 128);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            vTaskDelay(pdMS_TO_TICKS(80));
            }
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}