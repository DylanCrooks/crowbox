

/*
 * Servo control using ESP32 LEDC PWM
 * This code configures a servo motor to sweep from 0 to 180 degrees and back.
 * The servo is controlled using the LEDC peripheral of the ESP32.
 */

 /*

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
*/