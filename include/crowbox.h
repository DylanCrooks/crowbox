#include <stdint.h>
#include <stdbool.h>
#pragma once



// Training stage - controls state machine behavior
typedef enum {
    // Stage 1: Familiarity and basic depositing
    // Any valid weight accepted, preset thins gradually
    STAGE_1A_DISCOVERY,           // preset 100%, HVT 100%, no jackpot
    STAGE_1B_PRESET_75,           // preset 75%,  HVT 100%, no jackpot
    STAGE_1C_PRESET_50,           // preset 50%,  HVT 100%, no jackpot
    STAGE_1D_PRESET_25,           // preset 25%,  HVT 100%, no jackpot
    STAGE_1E_NO_PRESET,           // preset 0%,   HVT 100%, no jackpot
                                   // gate stays closed on PIR from here on

    // Stage 2: Shaping (object discrimination)
    // v1 = coin weight range only
    // future = CV-based criteria at increasing strictness via stage1_validation_strictness
    // continuous reinforcement required throughout, no jackpot
    STAGE_2_SHAPING,              // preset 0%, HVT 100%, jackpot 0%

    // Stage 3: VR thinning + jackpot introduction
    // jackpot introduced at start and held constant while HVT thins
    STAGE_3A_HVT_75,              // preset 0%, HVT 75%, jackpot 25%
    STAGE_3B_HVT_50,              // preset 0%, HVT 50%, jackpot 25%
    STAGE_3C_HVT_25,              // preset 0%, HVT 25%, jackpot 25%
    STAGE_3D_HVT_GONE,            // preset 0%, HVT 0%,  jackpot 25%


    STAGE_COUNT                    // always last — used for STAGE_CONFIGS array sizing
} training_stage_t;
//stage config - stores reward schedule and such. 

typedef enum { REWARD_TYPE_LVT, REWARD_TYPE_HVT, REWARD_TYPE_EHVT } reward_type_t;
typedef enum { TONE_REJECT, TONE_ACCEPT, TONE_JACKPOT } tone_id_t;




// All possible states the box can be in
typedef enum {
    STATE_IDLE,               // deep sleep, PIR watching
    STATE_PIR_TRIPPED,        // motion detected, waking up
    STATE_DEPOSIT_DETECTED,   // weight change detected
    STATE_VALIDATING,         // checking what type of object is on the platform (CV or weight validation)
    STATE_SIGNAL_VALID,          // object valid, green lights and reward sound
    STATE_SIGNAL_INVALID,        // object invalid, red lights and buzzer
    STATE_DISPENSING_SEED,         // object valid, feed tube servo fires
    STATE_DISPENSING_HIGH_VALUE_FOOD,         // object valid, feed tube servo fires
    STATE_DISPENSING_DRUGS,         // object valid, feed tube servo fires
    STATE_DISPENSING_OBJECT,       // dispensing object (association training)
    STATE_NO_REWARD,              // object valid but no food (variable ratio reward training)
    STATE_GATE_OPENING,         // feed chamber gate opens 
    STATE_GATE_CLOSING,        // feed chamber gate closing after delay, (send back to idle after)
    STATE_TRAPDOOR_UNLOCKING,   // solenoid retracts
    STATE_TRAPDOOR_OPENING,     // MG996R lifts door
    STATE_TRAPDOOR_CLOSING,
    STATE_TRAPDOOR_LOCKING,     // solenoid re-engages
    STATE_PRESET_WAIT,            // waiting for animal to leave before restocking
    STATE_LOGGING,            // sending event to Notecard
    STATE_COOLDOWN,           // brief reset
    STATE_ERROR               // sensor fault
} crowbox_state_t;

// All events that can move the state machine forward
typedef enum {
    EVENT_PIR_TRIPPED,               // motion detected, wake up, check stage, if stage 2, reward immediately, open gate, close gate, dispense reward again (w/out opening gate). if stage 3 or 4, wait for weight change
    EVENT_PIR_CLEARED,                    // animal left

    EVENT_DEPOSIT_WEIGHT_BASELINE,            // chamber empty
    EVENT_DEPOSIT_WEIGHT_DETECTED,            // weight changed above baseline, something is there
    EVENT_DEPOSIT_WEIGHT_SETTLED,             // weight stable for a few seconds, ready for classification

    EVENT_REWARD_TRAY_EMPTY,
    EVENT_REWARD_TRAY_LOADED,


    EVENT_OBJECT_VALID,              //classified as valid, sound and green light, roll for reward
    EVENT_OBJECT_INVALID,             //classified as invalid, buzzer and red light, no reward
                                    // go to CPU if valid, check stage, check schedule, do reward roll.
    EVENT_REWARD_GRANTED,             // schedule roll: pay out
    EVENT_REWARD_WITHHELD,
    EVENT_JACKPOT_GRANTED,             // jackpot reward (extra reward for valid object)

    EVENT_DISPENSE_COMPLETE,           
    EVENT_GATE_OPEN_COMPLETE,
    EVENT_GATE_CLOSE_COMPLETE,
    EVENT_TRAPDOOR_CLEAR_COMPLETE,

    EVENT_LOG_COMPLETE,
    EVENT_TIMEOUT,                        // go to idle after

    EVENT_HOPPER_EMPTY,          // value = hopper id (0-3)
    EVENT_BATTERY_LOW,
    EVENT_STAGE_CHANGED          // logging only
} crowbox_event_t;


/*
EVENT_PIR_CLEARED fires
→ STATE_AWAKE_WATCHING → STATE_PRESET_WAIT (new state)
→ wait 60-120 seconds (configurable, giving stragglers time to leave)
→ check reward chamber load cell — if empty, fire LVT servo
→ if stage 1, also trigger object dispenser (or flag for manual placement)
→ gate safety close if still open
→ STATE_IDLE (deep sleep)
*/

typedef struct { uint8_t dispenser_id; uint8_t dose_count; } reward_item_t;
typedef struct { reward_item_t items[4]; uint8_t item_count; } reward_bundle_t;
typedef struct {
    uint8_t preset_chance_pct;
    uint8_t hvt_chance_pct;
    uint8_t jackpot_chance_pct;
    bool    gate_opens_on_pir;
} stage_config_t;


extern const stage_config_t STAGE_CONFIGS[];
extern const size_t STAGE_CONFIGS_COUNT;

// A single event posted to the event bus
typedef struct {
    crowbox_event_t    type;
    int32_t             value;     // weight in grams, hopper id, etc — meaning depends on type
    int64_t             timestamp_ms;
} crowbox_event_msg_t;






