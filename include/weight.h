#pragma once
#include <stdint.h>



typedef enum {
    WEIGHT_SENSOR_DEPOSIT,   // HX711 #1 — GPIO32 (SCK), GPIO33 (DOUT)
    WEIGHT_SENSOR_REWARD     // HX711 #2 — GPIO25 (SCK), GPIO26 (DOUT)
} weight_sensor_t;

// weight_init
// Configure the SCK pin as output and DOUT pin as input with pull-up
// for the given sensor. Use PIN_HX711_1_SCK/DOUT or PIN_HX711_2_SCK/DOUT from pinout.h.
void weight_init(weight_sensor_t sensor);

// weight_read_raw
// Bit-bang the HX711 protocol:
//   1. Wait for DOUT to go LOW (signals data ready — poll with timeout)
//   2. Send 24 rising clock edges, reading DOUT after each rising edge (MSB first)
//   3. Send 1 final clock pulse (25 total = selects gain 128 for next reading)
//   4. Sign-extend the 24-bit two's complement result to int32_t
// Returns the raw ADC count.
int32_t weight_read_raw(weight_sensor_t sensor);

// weight_read_grams
// Convert raw ADC count to grams:
//   return (weight_read_raw(sensor) - tare_offset) / calibration_factor
// tare_offset and calibration_factor are placeholder constants — calibrate on hardware
//   with a known weight (e.g. tare empty, place 100g, record raw, divide: factor = raw/100).
float weight_read_grams(weight_sensor_t sensor);
