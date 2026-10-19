// UART protocol, vision side: public interface.
#pragma once

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "vision_protocol.h"

// Installs the UART driver, sends MSG_READY, and starts the link task.
void link_init(void);

// Waits up to `wait` ticks for a CLASSIFY or CAPTURE_REF job. Called by the
// vision worker; true if a job was copied into *job.
bool link_get_job(UART_msg_t *job, TickType_t wait);

// Frames and sends one message to the main ESP32. Safe from any task.
void link_send(const UART_msg_t *msg);
