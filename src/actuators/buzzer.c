#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "pinout.h"

void buzzer_init(void) {
        ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_1,
        .freq_hz = 2000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {
        .gpio_num = PIN_BUZZER,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHAN_BUZZER,
        .timer_sel = LEDC_TIMER_1,
        .duty = 512,
        .hpoint = 0
    };
    ledc_channel_config(&channel);
};

void buzzer_play_reject(void) {ESP_LOGI("buzzer", "playing reject sound");
        ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1, 200);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHAN_BUZZER, 128);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHAN_BUZZER);
        vTaskDelay(pdMS_TO_TICKS(300));
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHAN_BUZZER, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHAN_BUZZER);
}

void buzzer_play_accept(void) {
        ESP_LOGI("buzzer", "playing tone 2");
        ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1, 2000);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHAN_BUZZER, 128);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHAN_BUZZER);
        vTaskDelay(pdMS_TO_TICKS(200));
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHAN_BUZZER, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHAN_BUZZER);
}

void buzzer_play_jackpot(void) {
        ESP_LOGI("buzzer", "playing tone 3");
        uint32_t freqs[] = {400, 600, 800, 1000, 1400, 2000, 2800};
        for (int i = 0; i < 7; i++) {
            ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1, freqs[i]);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHAN_BUZZER, 128);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHAN_BUZZER);
            vTaskDelay(pdMS_TO_TICKS(80));
            }
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHAN_BUZZER, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHAN_BUZZER);
}