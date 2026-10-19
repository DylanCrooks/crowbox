#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "vision_protocol.h"

// UART link to the vision board (ESP32-P4), main ESP32 side.
// Wire format lives in shared/vision_protocol.h, used by both boards.

// Installs the UART driver. A failure is logged, not fatal: requests then
// just fail and callers fall back to weight-only.
void vision_link_init(void);

// Sends one request (MSG_PING / MSG_CLASSIFY / MSG_CAPTURE_REF) and waits up to
// timeout_ms for its final reply, skipping MSG_QUEUED. Replies with a different
// ID (stale replies, READY) are ignored. Returns false on timeout.
// Single caller at a time (the state machine task).
bool vision_link_request(uint8_t type, UART_msg_t *reply, uint32_t timeout_ms);

// Sends one request without waiting for any reply.
void vision_link_send(uint8_t type);
