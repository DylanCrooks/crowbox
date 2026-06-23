/**
 * @file pinout.h
 * @brief CrowBox GPIO pin assignments — canonical hardware reference for firmware.
 *
 * This file is the single source of truth for all pin numbers in firmware.
 * Never hardcode GPIO numbers elsewhere — always use these defines.
 * Update this file immediately whenever hardware wiring changes.
 *
 * Board: DOIT ESP32 DEVKIT V1 (HiLetgo clone, CP2102N, 38-pin, ESP-WROOM-32 module)
 * Framework: ESP-IDF + FreeRTOS
 *
 * Sync with: docs/pinout.md (schematic reference)
 *            
 */

#ifndef PINOUT_H
#define PINOUT_H

#include <stdint.h>

/* =============================================================================
 * SENSOR INPUTS — Native ESP32 GPIO
 * ============================================================================= */

/** PIR motion sensor (AM312). Input-only pin GPIO34 — RTC-capable, valid deep
 *  sleep wake source. AM312 is push-pull output so no pull-up resistor needed. */
#define PIN_PIR                 34

/** HX711 #1 — Deposit chamber load cell.
 *  Bit-bang protocol: SCK is output, DOUT is input.
 *  Configure internal pull-up on DOUT in firmware (gpio_set_pull_mode). */
#define PIN_HX711_1_SCK         32
#define PIN_HX711_1_DOUT        33

/** HX711 #2 — Reward chamber load cell.
 *  Same bit-bang requirements as HX711 #1. */
#define PIN_HX711_2_SCK         25
#define PIN_HX711_2_DOUT        26

/* =============================================================================
 * SERVO OUTPUTS — LEDC PWM, 50Hz, native ESP32 GPIO
 * All servos powered from 5V rail (buck converter output).
 * PWM signal is 3.3V logic from ESP32.
 * ============================================================================= */

/** SG90 doser servos — hopper mechanisms */
#define PIN_SERVO_HOPPER1       13      /* Hopper 1 — seed / LVT */
#define PIN_SERVO_HOPPER2       14      /* Hopper 2 — high-value / HVT */
#define PIN_SERVO_HOPPER3       18      /* Hopper 3 — drug / EHVT */
#define PIN_SERVO_HOPPER4       19      /* Hopper 4 — shaping object */

/** MG996R trapdoor lifter servo */
#define PIN_SERVO_TRAPDOOR      23

/* =============================================================================
 * BUZZER — LEDC PWM
 * Passive piezo — GPIO drives it directly (no separate VCC needed).
 * GPIO pin provides both signal and operating power for piezo element.
 * ============================================================================= */

#define PIN_BUZZER              4

/* =============================================================================
 * I2C BUS — Shared: MCP23017 GPIO expander + Notecard (future)
 * Fast mode recommended: 400kHz.
 * ============================================================================= */

#define PIN_I2C_SDA             21
#define PIN_I2C_SCL             22

/* =============================================================================
 * UART — Pi Zero communication (Phase 6+, not yet implemented)
 * UART2 peripheral, remapped to these pins via ESP32 GPIO matrix.
 * TX/RX naming is from ESP32's perspective.
 * ============================================================================= */

#define PIN_UART_TX             17      /* ESP32 TX → Pi Zero RX */
#define PIN_UART_RX             16      /* ESP32 RX ← Pi Zero TX */

/* =============================================================================
 * SPARE / RESERVED
 * ============================================================================= */

/** GPIO27 — spare output-capable GPIO, reserved for future expansion.
 *  Do not use without updating pinout.md and the KiCad schematic. */
#define PIN_SPARE_GPIO27        27

/* =============================================================================
 * I2C DEVICE ADDRESSES
 * ============================================================================= */

/** MCP23017 I2C address — A0/A1/A2 all tied to GND → base address 0x20 */
#define I2C_ADDR_MCP23017       0x20

/** Notecard I2C address (Phase 6+) */
#define I2C_ADDR_NOTECARD       0x17

/* =============================================================================
 * MCP23017 EXPANDER PIN ASSIGNMENTS
 * Accessed via I2C register writes to GPIOA (0x12) and GPIOB (0x13).
 * Use bit positions below with shadow register pattern — see firmware notes.
 *
 * Shadow register pattern (required — do not read-modify-write the chip):
 *   uint8_t mcp_gpioa_shadow = 0x00;
 *   uint8_t mcp_gpiob_shadow = 0x00;
 *   // To set a pin high:  shadow |=  (1 << MCP_BIT_xxx); write(shadow);
 *   // To set a pin low:   shadow &= ~(1 << MCP_BIT_xxx); write(shadow);
 * ============================================================================= */

/* --- Port A (GPIOA register, address 0x12) --- */

/** Hopper empty-detect IR beam sensors (input) — bits 0-3 of GPIOA.
 *  Configure as inputs via IODIRA register. */
#define MCP_BIT_IR_HOPPER1      0       /* GPA0 — Hopper 1 IR beam */
#define MCP_BIT_IR_HOPPER2      1       /* GPA1 — Hopper 2 IR beam */
#define MCP_BIT_IR_HOPPER3      2       /* GPA2 — Hopper 3 IR beam */
#define MCP_BIT_IR_HOPPER4      3       /* GPA3 — Hopper 4 IR beam */

/** Signal LEDs (output) — driven directly from MCP23017 via 330Ω series resistor */
#define MCP_BIT_LED_GREEN       4       /* GPA4 — Green LED (valid/accept) */
#define MCP_BIT_LED_RED         5       /* GPA5 — Red LED (invalid/reject) */
#define MCP_BIT_LED_BLUE        6       /* GPA6 — Blue LED (jackpot) */


/** COB LED strip (output) — via GODIY MOSFET module, 5V load */
#define MCP_BIT_COB_LED         7       /* GPA7 — White illumination strip */



/* --- Port B (GPIOB register, address 0x13) --- */

/** Gate linear actuator A — L298N IN1/IN2 (bits 0-1, same register for atomic write)
 *  Direction truth table:
 *    Extend:  IN1=1, IN2=0 → write 0bxxxxxx01 to GPIOB bits 1:0
 *    Retract: IN1=0, IN2=1 → write 0bxxxxxx10 to GPIOB bits 1:0
 *    Stop:    IN1=0, IN2=0 → write 0bxxxxxx00 to GPIOB bits 1:0 */
#define MCP_BIT_ACT_A_IN1       0       /* GPB0 — Actuator A IN1 */
#define MCP_BIT_ACT_A_IN2       1       /* GPB1 — Actuator A IN2 */

/** Gate linear actuator B — L298N IN3/IN4 (bits 2-3, same register for atomic write) */
#define MCP_BIT_ACT_B_IN1       2       /* GPB2 — Actuator B IN1 */
#define MCP_BIT_ACT_B_IN2       3       /* GPB3 — Actuator B IN2 */
/** Solenoid (output) — via GODIY MOSFET module (IRF520-style), 12V load */
#define MCP_BIT_SOLENOID        4       /* GPB4 — Solenoid logic line */

/** GPB5-7 — spare */
/* #define MCP_BIT_SPARE_GPB5   5 */
/* #define MCP_BIT_SPARE_GPB6   6 */
/* #define MCP_BIT_SPARE_GPB7   7 */

/* =============================================================================
 * MCP23017 REGISTER ADDRESSES (for direct I2C writes)
 * Using IOCON.BANK=0 (default) — registers are paired A/B sequentially.
 * ============================================================================= */

#define MCP_REG_IODIRA          0x00    /* Port A direction (1=input, 0=output) */
#define MCP_REG_IODIRB          0x01    /* Port B direction */
#define MCP_REG_GPIOA           0x12    /* Port A output latch */
#define MCP_REG_GPIOB           0x13    /* Port B output latch */
#define MCP_REG_OLATA           0x14    /* Port A output latch (alt access) */
#define MCP_REG_OLATB           0x15    /* Port B output latch (alt access) */

/* =============================================================================
 * LEDC PWM CHANNEL ASSIGNMENTS
 * ESP32 LEDC has 16 channels (0-15). Assign one per PWM output.
 * All servos: 50Hz, 16-bit resolution recommended (65536 steps).
 * Buzzer: frequency varies per tone, same channel assignment.
 * ============================================================================= */

#define LEDC_CHAN_SERVO_HOPPER1     0
#define LEDC_CHAN_SERVO_HOPPER2     1
#define LEDC_CHAN_SERVO_HOPPER3     2
#define LEDC_CHAN_SERVO_HOPPER4     3
#define LEDC_CHAN_SERVO_TRAPDOOR    4
#define LEDC_CHAN_BUZZER            5

/** Servo PWM timing constants (microseconds → duty cycle conversion needed in driver)
 *  Standard hobby servo: 50Hz period = 20ms = 20000us
 *  Neutral: 1500us, Full CW: 1000us, Full CCW: 2000us
 *  Calibrate these empirically per servo — SG90 and MG996R may differ slightly. */
#define SERVO_PWM_FREQ_HZ           50
#define SERVO_PULSE_MIN_US          1000    /* Full CW / 0° */
#define SERVO_PULSE_NEUTRAL_US      1500    /* Neutral / 90° */
#define SERVO_PULSE_MAX_US          2000    /* Full CCW / 180° */

/* =============================================================================
 * PINS TO NEVER USE — documented here to prevent accidental assignment
 * ============================================================================= */

/* GPIO 0, 2, 5, 12, 15 — strapping pins, boot behavior sensitive         */
/* GPIO 1, 3            — UART0 TX/RX, reserved for USB/serial monitor     */
/* GPIO 6-11            — internal SPI flash, using these crashes the chip  */
/* GPIO 34, 35, 36, 39  — input-only, no internal pull-up, no output drive  */
/*                         (GPIO34 used for PIR — AM312 push-pull is fine)  */

#endif /* PINOUT_H */