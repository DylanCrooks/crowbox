# CrowBox — Project Context for Claude Code

## What this project is

CrowBox is an embedded systems portfolio project: an ESP32-based device that trains wild crows, using operant conditioning, to deposit coins (and eventually trash/objects) in exchange for food. Built as a primary firmware/embedded portfolio piece for summer 2027 aerospace/defense internship applications. Beyond the device itself, the goal is to demonstrate real embedded skills: interrupt-driven sensor fusion, RTOS task management, low-power design, communication protocol implementation, and clean modular firmware architecture — the kind of work those roles actually test for in interviews.

See CLAUDE.local.md (gitignored, not in this repo) for personal/team context not relevant to anyone else reading this codebase.

## Current phase

Phase 1-3: building and testing individual sensor/actuator drivers on a breadboard, working toward a complete event-bus-driven state machine. No physical enclosure exists yet — that's Phase 9. All current work uses stand-in hardware (single sensors/servos on a breadboard) to prove firmware logic before any real mechanical assembly exists. See "Development workflow" below — this is a deliberate, established pattern, not a shortcut.

## Hardware architecture

**Brain:** ESP32 DevKitC (DOIT ESP32 DEVKIT V1), running ESP-IDF — not Arduino framework. See "Framework choice" below.

**Sensors:**
- 2x load cell + HX711 — one under the deposit chamber (object weight on drop), one under the reward chamber (detects when tray returns to empty, used to safely time gate closing without risk of closing on a beak, and to trigger re-staging the next treat)
- PIR motion sensor — mounted facing down into the deposit chamber from the underside of the shared floor/roof plate. Wake-from-deep-sleep trigger and general arrival signal. Deliberately does NOT distinguish crow from any other animal — treated as a generic presence trigger, not species detection.
- 4x IR beam-break sensors — one per hopper, detect whether that hopper is empty. Not used for primary deposit detection (that's weight-only).

**Actuators:**
- 4x SG90 servos — one per hopper (seed/LVT, high-value/HVT, drug, training object), each doses one portion into the shared central reward chamber. Doser mechanism (rotating disk vs trapdoor) is mechanical, not yet decided, doesn't affect firmware.
- 2x mini 12V linear actuators — raise/lower a sliding sleeve gate over the reward chamber. Lowered = inaccessible but visible (clear walls). Raised = accessible. Driven via L298N H-bridge. The 12V actuator rail is a fully separate power domain from 3.3V/5V logic — common ground only, never shared power rail (load cells are noise-sensitive to motor switching).

**I/O expansion:** MCP23017 I2C GPIO expander (address 0x20) carries all slow/non-timing-critical signals — hopper IR beams, signal lights, illumination, gate actuator direction. Native ESP32 GPIO is reserved for anything needing real PWM (servos, buzzer), precise bit-bang timing (HX711 clock/data), or RTC deep-sleep wake capability (PIR).

**Connectivity (Phase 5, not yet implemented):** Blues Wireless Notecard (LTE-M cellular), chosen specifically over raw AT-command modem programming to keep focus on firmware skills rather than modem vendor quirks. No WiFi anywhere — cellular/radio-first by design, since deployment should work without WiFi infrastructure.

**Computer vision (Phase 6+, not yet implemented):** Raspberry Pi Zero 2W acts as a vision coprocessor over UART (2 wires) to the ESP32. It owns the camera (Pi Camera v3) and runs classification entirely independently — the ESP32 only ever receives a short result string back, never touches image data. This split exists specifically so remote/LoRa deployment (no usable bandwidth for images) only needs to transmit a few bytes, not a photo.

**Power architecture:** single 12V LiFePO4 battery (standard 12V solar ecosystem compatible for Phase 9). 12V devices (2x linear actuators via L298N, solenoid via IRF520) wired directly to the battery — L298N's onboard logic-power jumper stays installed so it self-generates internal 5V, not tapped externally. One buck converter (12V→5V, 3A+) produces a single 5V rail feeding the ESP32 (VIN pin — its onboard regulator makes 3.3V internally, no separate 3.3V supply needed), PIR, all 5 servos, LED strip, buzzer, signal LEDs, and MCP23017 (tolerates 5V per datasheet). HX711 boards and IR beam sensors tap the ESP32's own 3.3V output pin. Total new conversion component for the whole system: one buck converter module. Verify actual solenoid/actuator max voltage tolerance against 4S LiFePO4's full-charge ceiling (~14.6V) once parts arrive — don't assume the "12V" label covers it.

## Framework choice: ESP-IDF, not Arduino

Deliberate. Arduino's framework wraps ESP-IDF and hides FreeRTOS behind a fake single-loop model. Using ESP-IDF directly means real FreeRTOS tasks, queues, and native peripheral calls are what's actually being built and can be spoken to in interviews. Practical consequence: common Arduino-only libraries (bogde/HX711, Adafruit_MCP23017) don't work directly — use an ESP-IDF-native equivalent or write a small driver from the datasheet.

## Firmware architecture

**Event-bus pattern.** Every hardware module (weight sensor, IR beam, PIR, servo, gate) is a self-contained driver with a narrow function-based interface and zero knowledge of the rest of the system. Drivers post events to a central FreeRTOS queue; they never call each other directly. The state machine is the only thing that consumes events and decides what happens next. Adding a new sensor later means writing a driver that posts a new event type — nothing else changes.

**File structure:**
```
src/
  main.c                 app_main(), task init
  state_machine.c        FSM logic — ALL event handlers live here
                          (handle_pir_tripped, handle_object_valid, etc.)
  event_bus.c             generic FreeRTOS queue wrapper, no domain logic
  sensors/
    weight.c              HX711 driver, instantiated for both load cells
    ir_beam.c
    pir.c
  actuators/
    servo.c                hopper dosers
    gate.c                  linear actuator gate control via L298N
    buzzer.c
  comms/
    notecard.c              Phase 5+
    lora.c                    Phase 8+, future
    comms.c                    shared interface, picks transport
  power/
    sleep.c
  config/
    config.c                  persistent config (training_stage_t, reward_schedule_t,
                               jackpot_chance_pct), NVS-backed
include/
  crowbox.h                   all shared enums/structs — see "Core types" below
```

**Development workflow — preserve this pattern.** Never write a new hardware driver directly inside the integrated state machine. Always build and prove it first as a standalone throwaway test program (a temporary `app_main()` that exercises just the one driver and prints results to serial). Only wire it into the real event bus once it works in isolation. This is the established working pattern for this project specifically so a single person can develop one component at a time without holding the entire system in their head.

**Header conventions:** `crowbox.h` only includes what it strictly needs to compile its own declarations (currently just `stdint.h`) — no framework-specific includes belong in shared type headers. Comments on enum values should be short noun phrases, not control-flow logic; control flow belongs in the handler functions in `state_machine.c`.

## Core types (snapshot — verify against the live file, this is known to be mid-revision)

```c
typedef enum {
    STAGE_1_PRESENCE_TRIGGER,   // PIR trip = LVT + shaping object dispensed together.
                                 // If shaping object is later deposited in the object chamber,
                                 // validate + reward (jackpot_chance_pct controls tier — set to
                                 // 100 for this stage: deposit of the shaping object is a concrete,
                                 // deterministic behavior, always rewarded at jackpot tier)
    STAGE_2_OBJECT_DEPOSIT       // valid object weight in chamber = reward (weight-range check,
                                 // no CV). jackpot_chance_pct set lower here — reward tier here is
                                 // probabilistic ("luck"), not behaviorally earned the way stage 1's is
} training_stage_t;

typedef enum {
    SCHEDULE_CONTINUOUS,
    SCHEDULE_RATIO_3_4,
    SCHEDULE_RATIO_1_2,
    SCHEDULE_RATIO_1_3
} reward_schedule_t;

typedef enum {
    REWARD_TIER_NONE,
    REWARD_TIER_STANDARD,
    REWARD_TIER_JACKPOT
} reward_tier_t;
```

No dedicated jackpot event type. `EVENT_REWARD_GRANTED` and `EVENT_DISPENSE_COMPLETE` both carry a `reward_tier_t` in their payload — the event type says a reward happened, the tier field says what kind. Both stages route through the identical `EVENT_OBJECT_VALID` → reward-decision pipeline; only `jackpot_chance_pct` in config differs between them (100 for stage 1, lower for stage 2), so no stage-conditional logic is needed at the event-handling level.

State and event enums (`crowbox_state_t`, `crowbox_event_t`, `crowbox_event_msg_t`) are actively evolving — read `include/crowbox.h` directly rather than trusting any snapshot here.

## Reward logic — resolved design

**Hopper assignments (confirmed, 4 hoppers total, no 5th hopper):**
```c
// Hopper 1 — LVT   — fires for PRESET (post-departure restock) and as part of JACKPOT bundle
// Hopper 2 — HVT   — fires for VOD and as part of JACKPOT bundle
// Hopper 3 — EHVT  — fires ONLY for JACKPOT
// Hopper 4 — shaping object — fires post-departure (stage 1 only), independent timing from hopper 1.
//             Manual placement by Dylan for now; automation deferred, will need simple
//             dispensed-vs-deposited count tracking to avoid floor pileup when built.
```

**Dispense reasons → reward types → feedback:**
```c
DISPENSE_REASON_PRESET   → hopper 1 (LVT) fires, silent, no gate
DISPENSE_REASON_VOD      → hopper 2 (HVT) fires, gate opens, TONE_ACCEPT
DISPENSE_REASON_JACKPOT  → hoppers 1+2+3 (LVT+HVT+EHVT) all fire, gate opens, TONE_JACKPOT
```
No `TONE_PRESET` — restocking happens while the box is unoccupied, nothing to play a sound for. `TONE_REJECT` is feedback without any dispense (invalid deposit).

**Gate-for-presence (corrected, now configurable):** PIR trip opens the gate to expose whatever is currently staged via a `config.presence_opens_gate` bool — does NOT trigger a new dispense either way. Default true for stage 1; stage 2 can be configured either way depending on what's being tested. Cheap to support both, so no hardcoded behavior per stage.

**Trapdoor clearing (deposit chamber):** the deposit chamber floor is a trapdoor that opens into a trash can below, using the existing deposit-chamber load cell (no new sensor) plus a 12V solenoid deadbolt (IRF520 MOSFET driver, switched from a spare MCP23017 pin) and an MG996R lifter servo (native GPIO23, needs its own PWM-capable pin like the other servos). Sequence: `STATE_TRAPDOOR_UNLOCKING → STATE_TRAPDOOR_OPENING → STATE_TRAPDOOR_CLOSING → STATE_TRAPDOOR_LOCKING`. Triggered at departure (`handle_pir_cleared`), gated on a `deposit_pending_clear` flag set whenever a deposit was logged during the visit (valid or rejected — either way the chamber needs clearing), not triggered immediately after logging so the mechanism doesn't operate while the crow might still be present.

Chamber lighting upgraded to a 5V COB LED strip (continuous, diffusion-free — aids future CV edge detection) — this is the realization of the previously-planned "white illumination" pin (GPA6 on the expander), not a new pin.

**PIR handling — discrete retrigger events, not a continuous presence signal.** Cheap PIR modules (HC-SR501 or clones) don't reliably hold a clean continuous-high signal for exactly as long as motion continues — behavior varies by jumper mode (repeatable vs. non-repeatable retrigger). Don't depend on raw signal duration. Instead, treat every PIR HIGH transition as a discrete event that updates a `last_pir_trigger_ms` timestamp; a periodic check (~1Hz) compares elapsed time against a configurable `DEPARTURE_DEBOUNCE_MS` (start around 30-60s) and fires `EVENT_PIR_CLEARED` once triggers have genuinely stopped. This is robust regardless of which PIR mode the module is in.

```c
static void handle_pir_tripped(void) {
    last_pir_trigger_ms = now_ms();
    if (gate_state == GATE_CLOSED) gate_open();
}

static void check_for_departure(void) {  // polled ~1Hz while awake
    if (now_ms() - last_pir_trigger_ms > DEPARTURE_DEBOUNCE_MS) {
        post_event(EVENT_PIR_CLEARED, ...);
    }
}

static void handle_pir_cleared(void) {
    if (weight_read_grams(SENSOR_REWARD_CHAMBER) <= BASELINE_THRESHOLD) {
        dispense(DISPENSER_LVT, DISPENSE_REASON_PRESET);
    }
    // stage 1: object dispensing hook point — manual for now, see hopper 4 note above
    if (gate_state == GATE_OPEN) gate_close();  // safety close if crow left mid-interaction
    transition_to(STATE_IDLE);
}
```

**Stage 2** (reliable depositing, thinned reinforcement): same gate-on-presence behavior as stage 1. On a validated deposit, roll `jackpot_chance_pct` (looked up from a decreasing threshold table keyed on cumulative successful-deposit count, tracked persistently in NVS — e.g. 50% for the first 20 successful deposits, 30% for the next 30, 15% after that; exact numbers tunable).

**Reward bundles, not single dispensers:**
```c
typedef struct {
    uint8_t dispenser_id;
    uint8_t dose_count;
} reward_item_t;

typedef struct {
    reward_item_t items[4];
    uint8_t item_count;
} reward_bundle_t;
```

**Stage 1 progressive validation strictness:** separate config field `stage1_validation_strictness` (independent of `training_stage_t`, same orthogonal-config pattern as schedule/stage). `classify_object()` reads this level — level 0 accepts anything above a near-zero noise floor, level 1 requires roughly "small object" weight range, level 2 requires matching the shaping object's specific profile, later levels are where real CV eventually plugs in. Bumped manually by Dylan based on logged behavior for v1. This is "successive approximation" in operant conditioning terms.

**Audio/visual feedback** — one passive buzzer (allows arbitrary PWM-driven tones, unlike an active buzzer which only produces one fixed tone):
```c
typedef enum { TONE_REJECT, TONE_ACCEPT, TONE_JACKPOT } tone_id_t;
```
Lights: green = accept (existing GPA4), red = reject (existing GPA5), blue = jackpot (NEW — needs adding to pinout, spare MCP23017 pin, from existing assorted LED pack).

**NVS (non-volatile storage):** part of ESP-IDF, uses a partition of the ESP32's own onboard flash — no additional hardware. Used for `training_stage_t`, `reward_schedule_t`, `stage1_validation_strictness`, cumulative successful-deposit count, and the jackpot threshold table — all need to survive reboot/power loss.

No new state types needed for any of the above — existing states and events already capture the sequence; reward/tone/light selection is a function call from within existing handlers in `state_machine.c`, dispatched on dispense reason and tier.

## Computer vision placeholder

`classify_object()` is the deliberate swap point for CV — currently stubbed. Two valid placeholder approaches discussed: return `true` unconditionally (simplest), or OpenCV's Hough Circle Transform on the Pi Zero side, which detects circular shapes with zero training data and works for "is there a coin here" without needing real chamber photos yet. The real trained classifier comes once Phase 9's physical chamber exists and real photos can be collected — the function signature shouldn't need to change when that swap happens.

## Open questions — confirm with Dylan, don't assume

- **IRF520 gate voltage:** breakout module is generally usable from 3.3V logic, but verify it reliably switches with the ESP32's 3.3V signal once wired — if not, a logic-level-specific MOSFET or a small buffer stage may be needed.
- **Solenoid/actuator voltage tolerance:** confirm against actual datasheets that they handle up to ~14.6V (4S LiFePO4 full charge), not just the nominal "12V" label.

@docs/pinout.md
@docs/partslist.md
@README.md