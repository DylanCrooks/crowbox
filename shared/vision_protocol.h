// Vision link protocol: message format and command IDs.
// Included by BOTH the main firmware (src/) and the vision firmware (vision/).
//
// Wire frame (fixed length, MSG_FRAME_LEN bytes):
//   [START_BYTE][type][ID][arg0][arg1][CRC-8/SMBUS over type..arg1]
// A reply carries the same ID as the request it answers.
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define START_BYTE    0xAA
#define MSG_FRAME_LEN 6

typedef enum {
    MSG_INVALID,        // never sent; catches zeroed/unset messages
    // main ESP32 -> P4
    MSG_PING,           // reply: OK
    MSG_CLASSIFY,       // reply: QUEUED then RESULT, or ERROR
    MSG_CAPTURE_REF,    // empty-pit background photo; reply: QUEUED then OK, or ERROR
    // P4 -> main ESP32
    MSG_READY,          // sent unprompted after boot, ID 0
    MSG_RESULT,         // arg0 = class, arg1 = confidence 0-99
    MSG_OK,             // request finished
    MSG_QUEUED,         // request accepted; the final reply follows with the same ID
    MSG_ERROR,          // arg0 = UART_error_enum
    // main ESP32 -> P4 (added last so the values above keep their numbers)
    MSG_SLEEP           // main is about to deep-sleep; reply: QUEUED then OK, then the P4 sleeps too
} UART_msg_enum;

typedef enum {
    ERR_NONE,
    ERR_SLEEP_FAILED,
    ERR_CAMERA_FAILED,
    ERR_SD_CARD_FAILED,
    ERR_BUSY,           // a CLASSIFY/CAPTURE_REF is already in progress; retry later
    ERR_UNKNOWN_REQUEST,
    ERR_CLASSIFIER_FAILED   // photo taken, but no classifier could run
} UART_error_enum;

// RESULT arg0. Placeholder classes until a real classifier exists.
typedef enum {
    CLASS_NONE,         // empty pit / nothing found
    CLASS_COIN,
    CLASS_OTHER         // something there, not a coin
} UART_class_enum;

typedef struct {
    uint8_t type;       // UART_msg_enum
    uint8_t ID;
    uint8_t arg0;       // meaning depends on type, see UART_msg_enum
    uint8_t arg1;
} UART_msg_t;

uint8_t crc8(const uint8_t *data, size_t len);
bool msg_pack(const UART_msg_t *msg, uint8_t out[MSG_FRAME_LEN]);
bool msg_unpack(const uint8_t in[MSG_FRAME_LEN], UART_msg_t *m);
