// UART protocol, vision side: public interface.
#pragma once

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "vision_protocol.h"

// Installs the UART driver, sends MSG_READY, and starts the link task.
void link_init(void);

// Waits up to `wait` ticks for the worker's next request: a job (CLASSIFY, CAPTURE_REF)
// or a control message (SLEEP), in arrival order, from one typed inbox. True if one was
// copied into *msg. Called by the vision worker.
// Jobs are admitted by a counting semaphore (LINK_MAX_JOBS tokens): no token -> the link
// task replies ERR_BUSY. Control messages need no token, so they're never refused.
bool link_get_request(UART_msg_t *msg, TickType_t wait);

// The worker finished a job (reply sent, photo saved): returns its token so the link
// task can admit the next one. Not for control messages.
void link_job_done(void);

// Blocks until everything handed to the UART has physically gone out on the wire.
// The worker calls it (through power_enter_deep_sleep) before sleeping, so the OK lands.
void link_flush(void);

// Frames and sends one message to the main ESP32. Safe from any task.
void link_send(const UART_msg_t *msg);
