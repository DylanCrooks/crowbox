# CrowBox GPIO Pinout

Canonical reference for all pin assignments. Update this immediately whenever wiring changes — this file is the source of truth, not memory.

Board: 38-pin ESP32 dev board (HiLetgo clone, CP2102N, ESP-WROOM-32 module). PlatformIO builds it with the `esp32doit-devkit-v1` board ID, which works because the chip is the same.

## Native ESP32 Pins

| GPIO | Function | Notes |
|---|---|---|
| 34 | PIR sensor input | RTC-capable — deep sleep wake source. Input-only pin; AM312 is push-pull so no pull-up resistor needed |
| 32 | HX711 #1 (deposit chamber) — SCK | Bit-bang timing, must be native output-capable GPIO |
| 33 | HX711 #1 (deposit chamber) — DOUT | Bit-bang timing, must be native. Configure internal pull-up in firmware |
| 25 | HX711 #2 (reward chamber) — SCK | Bit-bang timing, must be native output-capable GPIO |
| 26 | HX711 #2 (reward chamber) — DOUT | Bit-bang timing, must be native. Configure internal pull-up in firmware |
| 13 | MG996R servo — Hopper 1 (seed/LVT) doser | LEDC PWM |
| 19 | MG996R servo — Hopper 2 (high-value/HVT) doser | LEDC PWM. Moved from GPIO14, which outputs a signal at boot and would twitch the servo |
| 18 | MG996R servo — Hopper 3 (drug) doser | LEDC PWM |
| 14 | MG996R servo — Hopper 4 (object) doser | LEDC PWM. Hopper 4 is not part of the current build — pin reserved for later. GPIO14 outputs a signal at boot, acceptable here since nothing is attached |
| 23 | MG996R trapdoor lifter servo | LEDC PWM |
| 4 | Buzzer | LEDC PWM |
| 21 | I2C SDA | Shared bus: Notecard + MCP23017 |
| 22 | I2C SCL | Shared bus: Notecard + MCP23017 |
| 16 | UART RX ← vision board (ESP32-P4-NANO) TX | Phase 6+. UART2 default RX pin. Both boards are 3.3V logic, no level shifting. Runs over the Cat6 link (white/green) |
| 17 | UART TX → vision board (ESP32-P4-NANO) RX | Phase 6+. UART2 default TX pin. Runs over the Cat6 link (green) |
| 27 | Spare | Free output-capable GPIO, reserved for future expansion |

**15 of ~25 usable native GPIO in use. No free output-capable GPIO remaining except GPIO27 (spare). GPIO34/35/36/39 are input-only and cannot drive outputs.**

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
| GPA6 | Signal light - blue (jackpot) | 
| GPA7 | White illumination (plug into MOSFET for 5V LED strip) |
| GPB0 | Gate actuator A — IN1 | 
| GPB1 | Gate actuator A — IN2 |
| GPB2 | Gate actuator B — IN1 |
| GPB3 | Gate actuator B — IN2 | 
| GPB4 | Solenoid logic line, via D4184 MOSFET module | 
| GPB5 | SPARE
| GPB6 | SPARE
| GPB7 | SPARE
| GPB5-GPB7 | Spare (3 free) |

## Power Domains

| Rail | Source | Voltage | Powers |
|---|---|---|---|
| Main battery | LiFePO4 pack | 12V | Source for all downstream rails |
| Logic rail | Buck converter #1 (12V→5V) | 5V | ESP32 VIN (onboard AMS1117 then regulates to 3.3V internally), all 5 servos (MG996R ×5 — 4 hopper dosers + trapdoor lifter; ~2.5A stall each, so only one may move at a time — jackpot doses are staggered, never simultaneous), buzzer, COB LED strip (via D4184 MOSFET module), signal LEDs red/green/blue |
| Microcontroller rail | ESP32 onboard AMS1117 (from 5V VIN) | 3.3V | ESP32 core, MCP23017 (VDD), HX711 boards ×2, IR beam sensors ×4, PIR AM312 |
| Actuator rail | 12V direct from battery (post-fuse) | 12V | Linear actuators ×2 (via L298N), solenoid (via D4184 MOSFET module) |
| Buck converter #2 | TBD | TBD | Role not yet defined — clarify before build |

Common ground: all rails share a single GND reference tied at the battery negative terminal.

## Why the native/expander split

Native pins are reserved for: anything needing hardware PWM (servos, buzzer — no PWM peripheral on the expander), anything with precise bit-bang timing (HX711 clock/data), and the PIR wake source (deep sleep disables I2C, so an expander pin cannot wake the chip).

Everything on the expander is a simple on/off signal with no timing precision requirement — hopper beams, signal lights, illumination, actuator direction control.
