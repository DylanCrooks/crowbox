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
void weight_init(weight_sensor_t sensor) {}

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
int32_t weight_read_raw(weight_sensor_t sensor) { return 0; }

// weight_read_grams
// Pick the right tare offset for this sensor (deposit vs reward have different baselines).
// Formula: (weight_read_raw(sensor) - tare_offset) / CALIBRATION_FACTOR
// Returns float grams. Negative = lighter than tare (normal variation, treat as 0).
float weight_read_grams(weight_sensor_t sensor) { return 0.0f; }
