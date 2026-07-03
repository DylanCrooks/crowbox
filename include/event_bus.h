#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "crowbox.h"
#include "freertos/FreeRTOS.h"

void event_bus_init(void);
void event_bus_post(crowbox_event_msg_t msg);
void event_bus_post_from_isr(crowbox_event_msg_t msg);
bool event_bus_receive(crowbox_event_msg_t *msg, TickType_t ticks_to_wait);