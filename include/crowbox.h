#include <stdint.h>
#pragma once


typedef enum { STAGE_1_PRESENCE_TRIGGER, STAGE_2_OBJECT_DEPOSIT } training_stage_t;
typedef enum { SCHEDULE_CONTINUOUS, SCHEDULE_RATIO_3_4, SCHEDULE_RATIO_1_2, SCHEDULE_RATIO_1_3 } reward_schedule_t;
typedef enum { REWARD_TYPE_LVT, REWARD_TYPE_HVT, REWARD_TYPE_EHVT } reward_type_t;
typedef enum { DISPENSE_REASON_PRESET, DISPENSE_REASON_VOD, DISPENSE_REASON_JACKPOT } dispense_reason_t;
typedef enum { TONE_REJECT, TONE_ACCEPT, TONE_JACKPOT } tone_id_t;

typedef struct { uint8_t dispenser_id; uint8_t dose_count; } reward_item_t;
typedef struct { reward_item_t items[4]; uint8_t item_count; } reward_bundle_t;

typedef struct {
    crowbox_event_t     type;
    int32_t              value;
    weight_sensor_id_t    source;     // for WEIGHT_* events
    dispense_reason_t      reason;     // for DISPENSE_COMPLETE / REWARD_GRANTED events
    int64_t                 timestamp_ms;
} crowbox_event_msg_t;
// Training stage - controls state machine behavior


typedef enum {
    SCHEDULE_CONTINUOUS,   // every valid deposit rewarded
    SCHEDULE_RATIO_3_4,
    SCHEDULE_RATIO_1_2,
    SCHEDULE_RATIO_1_3
} reward_schedule_t;

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
    STATE_LOGGING,            // sending event to Notecard
    STATE_COOLDOWN,           // brief reset
    STATE_ERROR               // sensor fault
} crowbox_state_t;

// All events that can move the state machine forward
typedef enum {
    EVENT_PIR_TRIPPED,               // motion detected, wake up, check stage, if stage 2, reward immediately, open gate, close gate, dispense reward again (w/out opening gate). if stage 3 or 4, wait for weight change


    EVENT_WEIGHT_BASELINE,            // chamber empty
    EVENT_WEIGHT_DETECTED,            // weight changed above baseline, something is there
    EVENT_WEIGHT_SETTLED,             // weight stable for a few seconds, ready for classification

    EVENT_OBJECT_VALID,              //classified as valid, sound and green light, roll for reward
    EVENT_OBJECT_INVALID,             //classified as invalid, buzzer and red light, no reward
                                    // go to CPU if valid, check stage, check schedule, do reward roll.
    EVENT_REWARD_GRANTED,             // schedule roll: pay out, 
    EVENT_REWARD_WITHHELD,            // schedule roll: skip this time
    EVENT_JACKPOT_GRANTED,             // jackpot reward (extra reward for valid object)
    EVENT_JACKPOT_WITHHELD,            // jackpot reward withheld (extra reward for valid object)

    EVENT_DISPENSE_COMPLETE,           
    EVENT_GATE_OPEN_COMPLETE,
    EVENT_GATE_CLOSE_COMPLETE,

    EVENT_LOG_COMPLETE,
    EVENT_TIMEOUT,                    // animal left, no deposit

    EVENT_HOPPER_EMPTY,               // value field = which hopper (0-3)
    EVENT_BATTERY_LOW,
    EVENT_STAGE_CHANGED               // logging only, not control flow
} crowbox_event_t;

typedef enum {
    SENSOR_DEPOSIT_CHAMBER,
    SENSOR_REWARD_CHAMBER
} weight_sensor_id_t;

// A single event posted to the event bus
typedef struct {
    crowbox_event_t    type;
    int32_t             value;     // weight in grams, hopper id, etc — meaning depends on type
    weight_sensor_id_t  source;    // which load cell, only relevant for WEIGHT_* events
    int64_t             timestamp_ms;
} crowbox_event_msg_t;


