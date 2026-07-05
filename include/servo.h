#pragma once
#include <stdint.h>

// servo_init — configure LEDC_TIMER_0 (50Hz, 13-bit) and all 5 servo channels.
void servo_init(void);

// servo_set_angle — set servo channel (0-4) to the given angle (0-180).
void servo_set_angle(uint8_t servo_id, int angle);

// servo_doser_dispense — move hopper servo (0-3) to open angle, wait, return to closed.
void servo_doser_dispense(uint8_t hopper_id);

// servo_trapdoor_set_angle — set the trapdoor servo (channel 4) to the given angle.
void servo_trapdoor_set_angle(int angle);
