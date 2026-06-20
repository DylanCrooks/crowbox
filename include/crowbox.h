#include <stdint.h>
#pragma once

// Training stage - controls state machine behavior
typedef enum {
    STAGE_2_PRESENCE_TRIGGER,   // PIR trip alone = reward + object, if weight change detected, validate object and jackpot if valid
    STAGE_3_OBJECT_DEPOSIT,     // valid object weight in chamber = reward
    STAGE_4_SELF_SOURCED,      // valid object weight + CV validation = reward
} training_stage_t;

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


