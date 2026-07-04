// CLAUDE TODO: crowbox.h is included twice, remove one. Also remove stdio.h, stdint.h, task.h — not used here
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "event_bus.h"
#include "pinout.h"
#include "crowbox.h"

#define DEPARTURE_DEBOUNCE_MS 20000

static volatile int64_t s_last_trigger_microsec = 0;

static volatile bool s_crow_present;

static void IRAM_ATTR pir_isr(void *args) {
    s_last_trigger_microsec =  esp_timer_get_time();
    if (!s_crow_present) {
        s_crow_present = true;
        int64_t timestamp = s_last_trigger_microsec / 1000;
        crowbox_event_msg_t cb_pir_msg = {
            .timestamp_ms = timestamp,
            .type = EVENT_PIR_TRIPPED,
            .value = 0
        };
        event_bus_post_from_isr(cb_pir_msg); 

    }
};   // IRAM_ATTR: keep it, explained below

void pir_init(void){

    gpio_config_t pir_conf = {
        .pin_bit_mask = (1ULL << PIN_PIR),   // which pin(s) this config applies to — bitmask, not pin number directly
        .mode         = GPIO_MODE_INPUT,      // input only (GPIO34 can't output anyway)
        .pull_up_en   = GPIO_PULLUP_DISABLE,  // AM312 is push-pull, drives its own level — no pull needed
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_POSEDGE     // fire interrupt on rising edge (LOW→HIGH = motion detected)
    };
    gpio_config(&pir_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add((gpio_num_t)PIN_PIR, pir_isr, NULL);
    
};
void pir_check_departure(void){
    //poll every second to see if the time after crow's arrival is 
    if (!s_crow_present) {return;};
    int64_t current_time = esp_timer_get_time();
    if ((current_time - s_last_trigger_microsec) > (DEPARTURE_DEBOUNCE_MS * 1000)) {
        s_crow_present = false;
        crowbox_event_msg_t crow_depart_msg = {
            .timestamp_ms = (current_time * 1000),
            .type = EVENT_PIR_CLEARED,
            .value = 0,
        };
        event_bus_post(crow_depart_msg);
    };


};








    
