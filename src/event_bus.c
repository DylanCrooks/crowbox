#include "event_bus.h"
#include "freertos/queue.h"

static QueueHandle_t s_event_queue;


void event_bus_init(void) {
    s_event_queue = xQueueCreate(16,sizeof(crowbox_event_msg_t));
}

void event_bus_post(crowbox_event_msg_t msg) {
    xQueueSend(s_event_queue, &msg, 0);
}
void event_bus_post_from_isr(crowbox_event_msg_t msg) {
    xQueueSendFromISR(s_event_queue, &msg, NULL);
}

bool event_bus_receive(crowbox_event_msg_t *out, TickType_t ticks_to_wait) {
    return xQueueReceive(s_event_queue, out, ticks_to_wait) == pdPASS;
}