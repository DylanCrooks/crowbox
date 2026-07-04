#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "crowbox.h"
#include "stdint.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

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
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 2000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {
        .gpio_num = GPIO_NUM_4,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 512,
        .hpoint = 0
    };
    ledc_channel_config(&channel);
    while (1) { 
        
    }
    
}