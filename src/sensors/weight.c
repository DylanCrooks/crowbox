#include "weight.h"
#include "driver/gpio.h"
#include "pinout.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>
#include "esp_timer.h"

// CLAUDE TODO: fill in after calibration on hardware with a known reference weight
#define TARE_OFFSET_DEPOSIT     0
#define TARE_OFFSET_REWARD      0
#define CALIBRATION_FACTOR      1.0f   // raw counts per gram — measure empirically
#define TIMEOUT_MS 20000

// weight_init
// For the given sensor, configure:
//   SCK pin as GPIO_MODE_OUTPUT (gpio_set_direction)
//   DOUT pin as GPIO_MODE_INPUT with pull-up (gpio_set_direction + gpio_set_pull_mode)
// Use a helper or if/else to pick the right SCK/DOUT pair from pinout.h based on sensor.
void weight_init(weight_sensor_t sensor) {
    gpio_num_t dout_pin;
    gpio_num_t sck_pin;
    //does pin = PIN_HX711_x automatically cast PIN_HX711_x as gpio_num_t?
    if (sensor == WEIGHT_SENSOR_DEPOSIT){sck_pin = PIN_HX711_1_SCK; dout_pin = PIN_HX711_1_DOUT;};
    if (sensor == WEIGHT_SENSOR_REWARD){sck_pin = PIN_HX711_2_SCK; dout_pin = PIN_HX711_2_DOUT;};
    gpio_reset_pin(sck_pin);
    gpio_reset_pin(dout_pin);
    gpio_config_t weight_sck_conf = {
        .pin_bit_mask = (1ULL << (int)sck_pin), 
        .mode         = GPIO_MODE_OUTPUT,      
    };
    gpio_config_t weight_dout_conf = {
        .pin_bit_mask = (1ULL << (int)dout_pin), 
        .mode         = GPIO_MODE_INPUT,   
    };
    gpio_config(&weight_sck_conf);
    gpio_config(&weight_dout_conf);
    gpio_set_pull_mode(dout_pin, GPIO_PULLUP_ONLY);
    
}



// weight_read_raw
// Step 1: poll DOUT (gpio_get_level) until it goes LOW — HX711 signals ready.
//         Add a timeout loop so it doesn't hang forever if sensor is disconnected.
// Step 2: loop 24 times:
//           set SCK high (gpio_set_level)
//           read DOUT bit — shift into a uint32_t result MSB first: result = (result << 1) | bit
//           set SCK low
// Step 3: one more SCK pulse (high then low) to complete 25 pulses total (sets gain 128)
// Step 4: sign-extend 24-bit to int32_t:
//           if (result & 0x800000) result |= 0xFF000000;
//         This handles negative weights (load cell compressed rather than stretched).
int32_t weight_read_raw(weight_sensor_t sensor) {
    gpio_num_t dout_pin;
    gpio_num_t sck_pin;
    if (sensor == WEIGHT_SENSOR_DEPOSIT){sck_pin = PIN_HX711_1_SCK; dout_pin = PIN_HX711_1_DOUT;};
    if (sensor == WEIGHT_SENSOR_REWARD){sck_pin = PIN_HX711_2_SCK; dout_pin = PIN_HX711_2_DOUT;};
    uint32_t polling_start_time_ms = (esp_timer_get_time() / 1000);
    uint32_t current_time_ms;
    int32_t result_val = 0;
    bool exceeding_timeout = false;
    while (!exceeding_timeout){
        int dout_level = gpio_get_level(dout_pin);
        if (dout_level == 0) {
            // do logic shiz, break and end. 
            for (int i = 0; i < 24; i++) {
                gpio_set_level(sck_pin, 1);
                bool dout_val = gpio_get_level(dout_pin);
                result_val = (result_val << 1) | dout_val;
                gpio_set_level(sck_pin, 0);
            };
            //set gain 128 
            gpio_set_level(sck_pin, 1);
            gpio_set_level(sck_pin, 0);
            if (result_val & 0x800000) {result_val |= 0xFF000000;};
            break;
        }
        else if (dout_level != 0) {
            current_time_ms = (esp_timer_get_time() / 1000);
            if (current_time_ms - polling_start_time_ms > 200) {
                exceeding_timeout = true; 
            }
        };
    }
    return result_val;
};

// weight_read_grams
// Pick the right tare offset for this sensor (deposit vs reward have different baselines).
// Formula: (weight_read_raw(sensor) - tare_offset) / CALIBRATION_FACTOR
// raw = (grams * cf) + tare_offset
// Returns float grams. Negative = lighter than tare (normal variation, treat as 0).
float weight_read_grams(weight_sensor_t sensor) {
    int32_t raw = weight_read_raw(sensor);
    int32_t tare = (sensor == WEIGHT_SENSOR_DEPOSIT) ? TARE_OFFSET_DEPOSIT : TARE_OFFSET_REWARD;
    float ans = (raw - tare) / CALIBRATION_FACTOR;
    return ans;
};
