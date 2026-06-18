# crowbox

AI generated readme:

# CrowBox Firmware

Embedded firmware for an operant conditioning device that trains crows to deposit coins in exchange for food pellets. Built as a portfolio project targeting firmware engineering roles in aerospace and defense.

## What it does

The CrowBox uses a load cell weight sensor and IR beam break sensor to detect when a crow deposits a coin through a slot. On valid detection, a servo motor dispenses a food pellet. All events are logged via cellular (Blues Wireless Notecard over LTE-M) to a cloud backend.

Training operates in configurable stages:
- **Stage 2** — Crow presence on platform triggers food (weight threshold)
- **Stage 3** — Coin deposit through slot required for food (weight + IR validation)
- **Stage 4** — Crow must bring its own coin (behavioral, no firmware change required)

## Hardware

| Component | Purpose |
|---|---|
| ESP32 DevKitC | Main microcontroller, FreeRTOS firmware |
| Load cell (1kg) + HX711 amplifier | Coin deposit weight detection |
| IR beam break sensor | Coin slot passage detection |
| SG90 servo motor | Food dispenser mechanism |
| Passive buzzer | Audio feedback on dispense |
| Blues Wireless Notecard (LTE-M) | Cellular event logging |
| Solar panel + LiPo + TP4056 | Outdoor power management |

**Planned additions:** Raspberry Pi Zero 2W + Pi Camera v3 for onboard computer vision (coin/object classification). Google Coral Edge TPU for accelerated ML inference. RFM95W LoRa module for remote deployment without cellular coverage.

## Firmware architecture

Written in C using ESP-IDF with FreeRTOS. Key design patterns:

**Event-driven architecture** — modules communicate through a central FreeRTOS queue rather than calling each other directly. Adding a new sensor means writing a driver that posts events to the queue; nothing else changes.

**State machine** — IDLE → CROW_PRESENT → COIN_DEPOSITED → DISPENSING → LOGGING → COOLDOWN → IDLE. Stage 2 skips COIN_DEPOSITED. Stage switching via remote config update through Notecard, no reflash required.

**Modular comms layer** — Notecard and LoRa share a common `send_event()` interface. Swapping from cellular to LoRa for remote deployment requires no changes to application logic.

**Deep sleep power management** — ESP32 sleeps at ~10μA between visits, wakes on weight or PIR interrupt. System consumes roughly 5Wh/day, well within 5W solar panel output.

## Project structure

```
src/
  main.c              Entry point, task initialization
  state_machine.c     Core FSM logic and stage management
  event_bus.c         FreeRTOS queue wrapper (all inter-module comms)
  sensors/
    weight.c          HX711 load cell driver
    ir_beam.c         IR break beam driver
    pir.c             PIR motion sensor (wake trigger)
  actuators/
    servo.c           Food dispenser servo control
    buzzer.c          Audio feedback tones
  comms/
    notecard.c        Blues Wireless Notecard (LTE-M cellular)
    lora.c            RFM95W LoRa driver (future)
    comms.c           Shared interface, picks transport
  power/
    sleep.c           Deep sleep and wake configuration
  config/
    config.c          Persistent config (thresholds, stage, limits)
```

## Current status

**Phase 1 — Individual component drivers**

- [x] Development environment (VS Code + PlatformIO + ESP-IDF)
- [ ] HX711 weight sensor reading in grams
- [ ] IR beam break detection
- [ ] Servo control
- [ ] Buzzer tones

## Build and flash

Requires VS Code with the PlatformIO extension installed.

```bash
git clone https://github.com/YOURUSERNAME/crowbox.git
cd crowbox
# Open folder in VS Code
# PlatformIO installs dependencies automatically
# Connect ESP32 via USB-C, click Upload (→) in PlatformIO toolbar
```

Serial monitor: click the plug icon in the PlatformIO toolbar, 115200 baud.

## Roadmap

| Phase | Description | Status |
|---|---|---|
| 1 | Individual component drivers | In progress |
| 2 | Coin deposit detection → servo fires | Pending |
| 3 | Full state machine + FreeRTOS event bus | Pending |
| 4 | Deep sleep + power optimization | Pending |
| 5 | Blues Notecard cellular event logging | Pending |
| 6 | Raspberry Pi Zero 2W + Pi Camera v3 | Pending |
| 7 | Google Coral Edge TPU onboard inference | Pending |
| 8 | RFM95W LoRa for remote deployment | Pending |
| 9 | Physical enclosure + outdoor deployment | Pending |

## Hardware notes

- HX711 must be powered at 3.3V (not 5V) to keep data line ESP32-safe
- Servo powered from 5V rail, not ESP32's 3.3V pin
- 5V boost converter must be rated 2A+ when Pi Zero and Coral are added
- Pi Zero 2W and ESP32 both use 3.3V logic on UART — direct connection, no level shifting needed
