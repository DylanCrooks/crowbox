# CrowBox GPIO Pinout

Canonical reference for all pin assignments. Update this immediately whenever wiring changes — this file is the source of truth, not memory.

Board: DOIT ESP32 DEVKIT V1 (esp32doit-devkit-v1)

## Native ESP32 Pins

| GPIO | Function | Notes |
|---|---|---|
| 27 | PIR sensor input | RTC-capable — required for deep sleep wake source |
| 32 | HX711 #1 (deposit chamber) — SCK | Bit-bang timing, must be native |
| 33 | HX711 #1 (deposit chamber) — DOUT | Bit-bang timing, must be native |
| 25 | HX711 #2 (reward chamber) — SCK | Bit-bang timing, must be native |
| 26 | HX711 #2 (reward chamber) — DOUT | Bit-bang timing, must be native |
| 16 | Servo — Hopper 1 (seed/LVT) doser | LEDC PWM |
| 17 | Servo — Hopper 2 (high-value/HVT) doser | LEDC PWM |
| 18 | Servo — Hopper 3 (drug) doser | LEDC PWM |
| 19 | Servo — Hopper 4 (object) doser | LEDC PWM |
| 4 | Buzzer | LEDC PWM |
| 21 | I2C SDA | Shared bus: Notecard + MCP23017 |
| 22 | I2C SCL | Shared bus: Notecard + MCP23017 |
| 13 | UART TX → Pi Zero | Phase 6+ |
| 14 | UART RX ← Pi Zero | Phase 6+ |
| 23 | MG996R trapdoor lifter servo (PWM, needs native pin like the others)

**14 of ~25 usable native GPIO in use.**

Avoided: GPIO 0, 2, 5, 12, 15 (strapping pins), GPIO 1, 3 (UART0 — reserved for USB programming/serial monitor), GPIO 6-11 (internal SPI flash — never use).

## MCP23017 I/O Expander (I2C address 0x20)

Wiring to ESP32:

| MCP23017 pin | Connects to |
|---|---|
| VDD | ESP32 3.3V |
| VSS | GND |
| SDA | ESP32 GPIO21 |
| SCL | ESP32 GPIO22 |
| A0, A1, A2 | GND (sets address 0x20) |
| RESET | VDD (out of reset) |
| INTA, INTB | Unused (v1 uses polling, not interrupts) |

Expander pin assignments:

| Expander pin | Function |
|---|---|
| GPA0 | Hopper 1 empty-detect (IR beam) |
| GPA1 | Hopper 2 empty-detect (IR beam) |
| GPA2 | Hopper 3 empty-detect (IR beam) |
| GPA3 | Hopper 4 empty-detect (IR beam) |
| GPA4 | Signal light — green (valid) |
| GPA5 | Signal light — red (invalid) |
| GPA6 | White illumination (plug into MOSFET for 5V LED strip) |
| GPA7 | Gate actuator A — IN1 |
| GPB0 | Gate actuator A — IN2 |
| GPB1 | Gate actuator B — IN1 |
| GPB2 | Gate actuator B — IN2 |
| GPB3 | Signal light - blue (jackpot) |
| GPB4 | Solenoid logic line, via IRF520 |
| GPB5-GPB7 | Spare (3 free) |

## Power Domains

| Rail | Voltage | Powers |
|---|---|---|
| ESP32 onboard regulator | 3.3V | ESP32, MCP23017, HX711 boards, IR sensors, PIR |
| USB / breadboard supply | 5V | Servos, buzzer |
| Separate wall adapter / battery | 12V | Linear actuators only, via L298N — common ground with logic side, separate power rail |

## Why the native/expander split

Native pins are reserved for: anything needing hardware PWM (servos, buzzer — no PWM peripheral on the expander), anything with precise bit-bang timing (HX711 clock/data), and the PIR wake source (deep sleep disables I2C, so an expander pin cannot wake the chip).

Everything on the expander is a simple on/off signal with no timing precision requirement — hopper beams, signal lights, illumination, actuator direction control.
