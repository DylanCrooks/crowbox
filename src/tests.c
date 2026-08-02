#include "tests.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mcp23017.h"
#include "pinout.h"
#include "weight.h"
#include "solenoid.h"
#include "servo.h"
#include "gate.h"

void test_mcp23017_leds(void) {
    while (1) {
        mcp23017_set_pin(0, MCP_BIT_LED_GREEN, true);
        mcp23017_set_pin(0, MCP_BIT_LED_RED,   true);
        mcp23017_set_pin(0, MCP_BIT_LED_BLUE,  true);
        uint8_t val = mcp23017_read_port(0);
        ESP_LOGI("test_leds", "port A: 0x%02X", val);
        vTaskDelay(pdMS_TO_TICKS(500));
        mcp23017_set_pin(0, MCP_BIT_LED_GREEN, false);
        mcp23017_set_pin(0, MCP_BIT_LED_RED,   false);
        mcp23017_set_pin(0, MCP_BIT_LED_BLUE,  false);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void test_weight_sensors(void) {
    while (1) {
        int32_t raw_dep = weight_read_raw(WEIGHT_SENSOR_DEPOSIT);
        int32_t raw_rew = weight_read_raw(WEIGHT_SENSOR_REWARD);
        ESP_LOGI("test_weight", "deposit raw=%ld | reward raw=%ld", raw_dep, raw_rew);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void test_solenoid_and_servo(void) {
    int cycle = 0;
    while (1) {
        if (cycle % 10 == 0) {
            ESP_LOGI("test_solenoid", "unlocking");
            solenoid_unlock();
            vTaskDelay(pdMS_TO_TICKS(450));
            ESP_LOGI("test_solenoid", "locking");
            solenoid_lock();
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        if (cycle % 5 == 0) {
            ESP_LOGI("test_servo", "trapdoor -> 0 deg");
            servo_trapdoor_set_angle(0);
            vTaskDelay(pdMS_TO_TICKS(1000));
            ESP_LOGI("test_servo", "trapdoor -> 90 deg");
            servo_trapdoor_set_angle(90);
            vTaskDelay(pdMS_TO_TICKS(1000));
            ESP_LOGI("test_servo", "trapdoor -> 0 deg");
            servo_trapdoor_set_angle(0);
        }

        cycle++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void test_gate(void) {
    while (1) {
        ESP_LOGI("test_gate", "opening");
        gate_open();   // blocks for GATE_TRAVEL_TICKS then stops — tune that define on hardware
        vTaskDelay(pdMS_TO_TICKS(2000));

        ESP_LOGI("test_gate", "closing");
        gate_close();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
