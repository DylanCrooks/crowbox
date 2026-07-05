#include "weight.h"
#include "driver/gpio.h"
#include "pinout.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// CLAUDE TODO: fill in after calibration on hardware with a known reference weight
#define TARE_OFFSET_DEPOSIT     0
#define TARE_OFFSET_REWARD      0
#define CALIBRATION_FACTOR      1.0f   // raw counts per gram — measure empirically

// weight_init
// For the given sensor, configure:
//   SCK pin as GPIO_MODE_OUTPUT (gpio_set_direction)
//   DOUT pin as GPIO_MODE_INPUT with pull-up (gpio_set_direction + gpio_set_pull_mode)
// Use a helper or if/else to pick the right SCK/DOUT pair from pinout.h based on sensor.
void weight_init(weight_sensor_t sensor) {
    // CLAUDE TODO: declare gpio_num_t sck_pin, dout_pin
    // CLAUDE TODO: if (sensor == WEIGHT_SENSOR_DEPOSIT) assign PIN_HX711_DEPOSIT_SCK / _DOUT
    //              else assign PIN_HX711_REWARD_SCK / _DOUT (from pinout.h)
    // CLAUDE TODO: gpio_reset_pin(sck_pin) and gpio_reset_pin(dout_pin) — clears any previous config
    // CLAUDE TODO: gpio_set_direction(sck_pin, GPIO_MODE_OUTPUT)
    // CLAUDE TODO: gpio_set_direction(dout_pin, GPIO_MODE_INPUT)
    // CLAUDE TODO: gpio_set_pull_mode(dout_pin, GPIO_PULLUP_ONLY) — DOUT floats when idle, need pull-up
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
    // CLAUDE TODO: pick sck_pin / dout_pin from sensor param (same if/else as weight_init)
    // CLAUDE TODO: timeout loop — poll gpio_get_level(dout_pin) until it returns 0 (LOW = ready)
    //              use a counter: for (int timeout = 0; timeout < 100000; timeout++) { if (!level) break; }
    //              return 0 if it never goes LOW — sensor not connected or powered
    // CLAUDE TODO: declare uint32_t result = 0;
    // CLAUDE TODO: 24-pulse read loop:
    //              gpio_set_level(sck_pin, 1);
    //              uint32_t bit = gpio_get_level(dout_pin);
    //              gpio_set_level(sck_pin, 0);
    //              result = (result << 1) | bit;
    // CLAUDE TODO: 25th pulse (sets gain 128 for next read — required by HX711 protocol):
    //              gpio_set_level(sck_pin, 1); gpio_set_level(sck_pin, 0);
    // CLAUDE TODO: sign-extend: if (result & 0x800000) result |= 0xFF000000;
    //              Why: bit 23 is the sign bit in 24-bit two's complement.
    //              Without this, a negative reading (lighter than tare) would look like a huge positive number.
    // CLAUDE TODO: return (int32_t)result;
    return 0;
}

// weight_read_grams
// Pick the right tare offset for this sensor (deposit vs reward have different baselines).
// Formula: (weight_read_raw(sensor) - tare_offset) / CALIBRATION_FACTOR
// Returns float grams. Negative = lighter than tare (normal variation, treat as 0).
float weight_read_grams(weight_sensor_t sensor) {
    // CLAUDE TODO: int32_t raw = weight_read_raw(sensor);
    // CLAUDE TODO: int32_t tare = (sensor == WEIGHT_SENSOR_DEPOSIT) ? TARE_OFFSET_DEPOSIT : TARE_OFFSET_REWARD;
    // CLAUDE TODO: return (raw - tare) / CALIBRATION_FACTOR;
    return 0.0f;
}
