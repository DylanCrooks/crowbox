// CLAUDE TODO: remove stdio.h, crowbox.h, stdint.h, gpio.h — servo.c doesn't use any of them
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "pinout.h"


#define DISPENSE_ANGLE 1
// CLAUDE TODO: wrap in pdMS_TO_TICKS() so it's explicitly milliseconds: pdMS_TO_TICKS(1000)
#define DISPENSE_TICKS 1000
#define CLOSED_ANGLE 0 
/*
 * Servo control using ESP32 LEDC PWM
 * This code configures a servo motor to sweep from 0 to 180 degrees and back.
 * The servo is controlled using the LEDC peripheral of the ESP32.
 */

static uint32_t angle_to_duty(int angle) {
        uint32_t us = SERVO_PULSE_MIN_US + (angle * (SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US) / 180);
        return (us * 8191) / 20000;
    };

void servo_set_angle(uint8_t hopper_channel_id, int angle) {
    uint32_t duty = angle_to_duty(angle);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, hopper_channel_id, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, hopper_channel_id);
};
// CLAUDE TODO: tune DISPENSE_ANGLE and DISPENSE_TICKS on hardware
void servo_doser_dispense(uint8_t hopper_id) {
    servo_set_angle(hopper_id, DISPENSE_ANGLE);
    vTaskDelay(DISPENSE_TICKS);
    servo_set_angle(hopper_id, CLOSED_ANGLE);

};

// servo_trapdoor_set_angle
// Same as servo_set_angle but hardcoded to LEDC_CHAN_SERVO_TRAPDOOR (channel 4).
// Called by solenoid.c's trapdoor sequence — not for hopper dosers.
// CLAUDE TODO: define TRAPDOOR_OPEN_ANGLE and TRAPDOOR_CLOSED_ANGLE — tune on hardware.
void servo_trapdoor_set_angle(int angle) {}

void servo_init(void) {
ledc_timer_config_t timer = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .duty_resolution = LEDC_TIMER_13_BIT,
    .timer_num = LEDC_TIMER_0,
    .freq_hz = 50,
    .clk_cfg = LEDC_AUTO_CLK
};
ledc_timer_config(&timer);
// have one timer for all 5 servos

ledc_channel_config_t channel_servo1 = {
    .gpio_num = PIN_SERVO_HOPPER1,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHAN_SERVO_HOPPER1,
    .timer_sel = LEDC_TIMER_0,
    .duty = angle_to_duty(0),
    .hpoint = 0
};
ledc_channel_config(&channel_servo1);

ledc_channel_config_t channel_servo2 = {
    .gpio_num = PIN_SERVO_HOPPER2,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHAN_SERVO_HOPPER2,
    .timer_sel = LEDC_TIMER_0,
    .duty = angle_to_duty(0),
    .hpoint = 0
};
ledc_channel_config(&channel_servo2);

ledc_channel_config_t channel_servo3 = {
    .gpio_num = PIN_SERVO_HOPPER3,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHAN_SERVO_HOPPER3,
    .timer_sel = LEDC_TIMER_0,
    .duty = angle_to_duty(0),
    .hpoint = 0
};
ledc_channel_config(&channel_servo3);

ledc_channel_config_t channel_servo4 = {
    .gpio_num = PIN_SERVO_HOPPER4,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHAN_SERVO_HOPPER4,
    .timer_sel = LEDC_TIMER_0,
    .duty = angle_to_duty(0),
    .hpoint = 0
};
ledc_channel_config(&channel_servo4);

ledc_channel_config_t channel_servo_trapdoor = {
    .gpio_num = PIN_SERVO_TRAPDOOR,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHAN_SERVO_TRAPDOOR,
    .timer_sel = LEDC_TIMER_0,
    .duty = angle_to_duty(0),
    .hpoint = 0
};
ledc_channel_config(&channel_servo_trapdoor);
};