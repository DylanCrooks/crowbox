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

// ---------------------------------------------------------------------------
// Junction box bring-up: I2C scan, both MOSFET modules, both L298N channels,
// deposit-pit HX711. Runs each step on a timer with a countdown in the log,
// so you can hold the multimeter probes in place. Ends in a live HX711 loop.
// Press EN (reset) to run the whole sequence again.
// ---------------------------------------------------------------------------
#include "i2c_bus.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"

#define BRINGUP_TAG "bringup"

static void bringup_countdown(const char *what, int seconds) {
    for (int s = seconds; s > 0; s--) {
        ESP_LOGI(BRINGUP_TAG, "    %s ... %d s", what, s);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void bringup_mosfet(const char *name, bool port, uint8_t bit) {
    ESP_LOGI(BRINGUP_TAG, "%s MOSFET: OFF. Measure across the load: expect ~0 V", name);
    mcp23017_set_pin(port, bit, false);
    bringup_countdown("OFF", 5);
    ESP_LOGI(BRINGUP_TAG, "%s MOSFET: ON. Measure across the load: expect ~12 V", name);
    mcp23017_set_pin(port, bit, true);
    bringup_countdown("ON", 5);
    mcp23017_set_pin(port, bit, false);
    ESP_LOGI(BRINGUP_TAG, "%s MOSFET: OFF again", name);
    vTaskDelay(pdMS_TO_TICKS(2000));
}

static void bringup_actuator(const char *name, uint8_t in1, uint8_t in2) {
    ESP_LOGI(BRINGUP_TAG, "Actuator %s: EXTEND (IN1=1 IN2=0). L298N output: expect ~+10 V", name);
    mcp23017_set_pin(1, in2, false);
    mcp23017_set_pin(1, in1, true);
    bringup_countdown("extend", 3);
    mcp23017_set_pin(1, in1, false);
    ESP_LOGI(BRINGUP_TAG, "Actuator %s: STOP. Expect ~0 V", name);
    bringup_countdown("stop", 2);
    ESP_LOGI(BRINGUP_TAG, "Actuator %s: RETRACT (IN1=0 IN2=1). Expect ~-10 V (polarity flipped)", name);
    mcp23017_set_pin(1, in2, true);
    bringup_countdown("retract", 3);
    mcp23017_set_pin(1, in2, false);
    ESP_LOGI(BRINGUP_TAG, "Actuator %s: STOP", name);
    vTaskDelay(pdMS_TO_TICKS(2000));
}

void test_junction_bringup(void) {
    ESP_LOGI(BRINGUP_TAG, "=== Junction box bring-up ===");

    // 1. I2C scan. Expect 0x20 (MCP23017) and 0x17 (Notecard).
    i2c_master_bus_handle_t bus = i2c_bus_get_handle();
    int found = 0;
    ESP_LOGI(BRINGUP_TAG, "[1/4] I2C scan 0x08-0x77 ...");
    for (uint16_t addr = 0x08; addr <= 0x77; addr++) {
        if (i2c_master_probe(bus, addr, 50) == ESP_OK) {
            const char *who = (addr == 0x20) ? "MCP23017" : (addr == 0x17) ? "Notecard" : "unexpected";
            ESP_LOGI(BRINGUP_TAG, "    found 0x%02X  (%s)", addr, who);
            found++;
        }
    }
    ESP_LOGI(BRINGUP_TAG, "    %d device(s) found", found);

    mcp23017_init();
    gate_stop();                                   // all four L298N inputs low
    mcp23017_set_pin(1, MCP_BIT_SOLENOID, false);
    mcp23017_set_pin(0, MCP_BIT_COB_LED, false);
    // Set up the deposit HX711 pins by hand. weight_init() would also start the
    // background read task, which posts to the event bus (not created in this test)
    // and would fight this test's read loop for the same pins.
    gpio_config_t hx_sck  = { .pin_bit_mask = 1ULL << PIN_HX711_1_SCK,  .mode = GPIO_MODE_OUTPUT };
    gpio_config_t hx_dout = { .pin_bit_mask = 1ULL << PIN_HX711_1_DOUT, .mode = GPIO_MODE_INPUT,
                              .pull_up_en = GPIO_PULLUP_ENABLE };
    gpio_config(&hx_sck);
    gpio_config(&hx_dout);
    gpio_set_level(PIN_HX711_1_SCK, 0);
    vTaskDelay(pdMS_TO_TICKS(3000));

    // 2. MOSFET modules
    ESP_LOGI(BRINGUP_TAG, "[2/4] MOSFET modules");
    bringup_mosfet("Solenoid (GPB4)", 1, MCP_BIT_SOLENOID);
    bringup_mosfet("COB LED strip (GPA7)", 0, MCP_BIT_COB_LED);

    // 3. L298N, one channel at a time
    ESP_LOGI(BRINGUP_TAG, "[3/4] L298N actuators");
    bringup_actuator("A (GPB0/GPB1)", MCP_BIT_ACT_A_IN1, MCP_BIT_ACT_A_IN2);
    bringup_actuator("B (GPB2/GPB3)", MCP_BIT_ACT_B_IN1, MCP_BIT_ACT_B_IN2);
    gate_stop();

    // 4. Deposit-pit HX711, then loop forever
    ESP_LOGI(BRINGUP_TAG, "[4/4] Deposit HX711 (GPIO32/33). Press on the pit to see raw counts move.");
    ESP_LOGI(BRINGUP_TAG, "Sequence done. Press EN to run it again.");
    while (1) {
        int32_t raw = weight_read_raw(WEIGHT_SENSOR_DEPOSIT);
        ESP_LOGI(BRINGUP_TAG, "    HX711 raw: %ld", (long)raw);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
