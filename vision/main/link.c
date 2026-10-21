// UART protocol, vision side: receive requests from the main ESP32, send results.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "link.h"

// Placeholder pins: confirm both are on the header and unused (see the avoid
// list in docs/crowbox_vision_box.txt) when the board arrives, then record
// them in docs/pinout.md.
#define LINK_TXD (GPIO_NUM_4)   // -> main ESP32 RX (GPIO16)
#define LINK_RXD (GPIO_NUM_5)   // <- main ESP32 TX (GPIO17)
#define LINK_RTS (UART_PIN_NO_CHANGE)
#define LINK_CTS (UART_PIN_NO_CHANGE)

#define LINK_UART_PORT_NUM      (UART_NUM_1)    // UART0 is the console
#define LINK_UART_BAUD_RATE     (115200)
#define LINK_TASK_STACK_SIZE    (4096)          // bytes (ESP-IDF counts bytes, not words)
#define LINK_TASK_PRIORITY      (10)
#define TIMEOUT                 pdMS_TO_TICKS(20)
#define LINK_MAX_JOBS           (1)     // jobs queued or running at once; more get ERR_BUSY
#define LINK_INBOX_LEN          (4)     // > LINK_MAX_JOBS, so control messages always fit
static const char *TAG = "link";

#define READ_BUF_SIZE (1)
#define BUF_SIZE (1024)

static QueueHandle_t inbox;
static SemaphoreHandle_t job_slots;     // counting semaphore: one token per job allowed in flight


static void LINK_task(void *arg);

void link_init(void){
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = LINK_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(LINK_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(LINK_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(LINK_UART_PORT_NUM, LINK_TXD, LINK_RXD, LINK_RTS, LINK_CTS));

    UART_msg_t m = {
        .type = MSG_READY,
        .ID = 0,
        .arg0 = 0,
        .arg1 = 0,
    };
    link_send(&m);
    inbox = xQueueCreate(LINK_INBOX_LEN, sizeof(UART_msg_t));   // e.g. 4: jobs + control
    job_slots = xSemaphoreCreateCounting(LINK_MAX_JOBS, LINK_MAX_JOBS);   // e.g. 1, 1
    xTaskCreate(LINK_task, "uart_LINK_task", LINK_TASK_STACK_SIZE, NULL, LINK_TASK_PRIORITY, NULL);

}

static UART_msg_t uart_validate(uint8_t *data) {
    uint8_t in[MSG_FRAME_LEN];
    uint8_t count = 0;
    while (1) {
        while (count < MSG_FRAME_LEN) {
            while (count == 0) {
            int len = uart_read_bytes(LINK_UART_PORT_NUM, data, 1, portMAX_DELAY);
            if (len > 0) {
                if (data[len-1] == START_BYTE) {in[0] = data[len-1]; count++;}}
            else { ESP_LOGW(TAG, "error or timeout on read_bytes in uart_validate"); 
            continue;}
            }
            int len = uart_read_bytes(LINK_UART_PORT_NUM, data, 1, TIMEOUT);
            if (len > 0) {
            in[count] = data[0];
            count++;}
            if (len == 0) {count = 0;}
            if (len == -1) {count = 0;}
        }
        UART_msg_t m;
        if (msg_unpack(in, &m)) {return m;}
        else {
            for (uint8_t i = 1; i < MSG_FRAME_LEN; i++) {
                if (in[i] == START_BYTE) {
                    memmove(in, in + i, 6 - i);
                count = 6 - i; 
                break;}
                count = 0;
                }
            }
    }
}



static UART_msg_t handle_msg(UART_msg_t *m) {
    UART_msg_t reply = {0};

   
    switch(m->type) {
        default:
        reply.type = MSG_ERROR;
        reply.ID = m->ID;
        reply.arg0 = ERR_UNKNOWN_REQUEST;
        reply.arg1 = 0;
        break; 

        case MSG_PING:
        reply.type = MSG_OK;
        reply.ID = m->ID;
        reply.arg0 = 0;
        reply.arg1 = 0;
        break;

        case MSG_CLASSIFY: {
        if (xSemaphoreTake(job_slots, 0) == pdTRUE) {
            if (xQueueSend(inbox,m,0) == pdTRUE) {
            reply.type = MSG_QUEUED;
            reply.ID = m->ID;
            reply.arg0 = 0;
            reply.arg1 = 0;
            } else {
                xSemaphoreGive(job_slots);
                reply.type = MSG_ERROR;
                reply.ID = m->ID;
                reply.arg0 = ERR_BUSY;
                reply.arg1 = 0;
            }
        } else {
        
        reply.type = MSG_ERROR;
        reply.ID = m->ID;
        reply.arg0 = ERR_BUSY;
        reply.arg1 = 0; }
        }
        break;
        

        case MSG_CAPTURE_REF:
        if (xSemaphoreTake(job_slots, 0) == pdTRUE) {
            if (xQueueSend(inbox,m,0) == pdTRUE) {
            reply.type = MSG_QUEUED;
            reply.ID = m->ID;
            reply.arg0 = 0;
            reply.arg1 = 0;
            } else {
                xSemaphoreGive(job_slots);
                reply.type = MSG_ERROR;
                reply.ID = m->ID;
                reply.arg0 = ERR_BUSY;
                reply.arg1 = 0;
            }
        } else {
            reply.type = MSG_ERROR;
            reply.ID = m->ID;
            reply.arg0 = ERR_BUSY;
            reply.arg1 = 0;
        }
        break;

        case MSG_SLEEP:
        if (xQueueSend(inbox,m,0) == pdTRUE) {
        reply.type = MSG_QUEUED;
        reply.ID = m->ID;
        reply.arg0 = 0;
        reply.arg1 = 0;
        } else {
            // should never happen
            reply.type = MSG_ERROR;
            reply.ID = m->ID;
            reply.arg0 = ERR_SLEEP_FAILED;
            reply.arg1 = 0;
        }
        break;
    }

    return reply;
}




void link_send(const UART_msg_t *msg){
    uint8_t out[MSG_FRAME_LEN];
        msg_pack(msg, out);
        uart_write_bytes(LINK_UART_PORT_NUM, out, MSG_FRAME_LEN);
}

static void LINK_task(void *arg)
{
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(READ_BUF_SIZE);
    while (1) {
        // Read data from the UART
        UART_msg_t received = uart_validate(data);
        ESP_LOGI(TAG, "rx type=%u id=%u arg0=%u arg1=%u",
                 received.type, received.ID, received.arg0, received.arg1);
        UART_msg_t reply = handle_msg(&received);
        link_send(&reply);
        
    }
}

/* Typed inbox: inbox becomes the worker's one inbox for jobs AND control messages
 * (consider renaming it inbox). How many jobs may be in flight is decided by job_slots,
 * not by the queue's length, so a control message always finds room.
 *
 * link_init(), before xTaskCreate:
 *   inbox = xQueueCreate(LINK_INBOX_LEN, sizeof(UART_msg_t));   // e.g. 4: jobs + control
 *   job_slots = xSemaphoreCreateCounting(LINK_MAX_JOBS, LINK_MAX_JOBS);   // e.g. 1, 1
 *
 * handle_msg: dispatch by category, e.g. handle_job_msg() / handle_ctrl_msg():
 *   job (CLASSIFY, CAPTURE_REF):
 *     xSemaphoreTake(job_slots, 0) == pdTRUE  -> xQueueSend(inbox, m, 0), reply QUEUED
 *     otherwise                                -> reply ERR_BUSY (stale work is refused)
 *   control (SLEEP):
 *     xQueueSend(inbox, m, 0), reply QUEUED. No token: never refused while the inbox
 *     has more slots than LINK_MAX_JOBS.
 *   PING stays answered right in the link task.
 */

/* link_get_request: xQueueReceive(inbox, msg, wait) == pdTRUE. FIFO order means a SLEEP
 * that arrives mid-job runs after that job. */
bool link_get_request(UART_msg_t *msg, TickType_t wait)
{
    if (xQueueReceive(inbox,msg,wait) == pdTRUE) {
        return true;
    } else {
        return false;
    }
    
}

/* link_job_done: the worker finished a job (reply sent, photo saved): give its token back,
 * xSemaphoreGive(job_slots). Not called for control messages: they never took one. */
void link_job_done(void) {
    xSemaphoreGive(job_slots);
}

/* link_flush
 * uart_wait_tx_done(LINK_UART_PORT_NUM, timeout): blocks until the TX FIFO is empty.
 * 6 bytes at 115200 baud take ~0.5 ms; a 20-50 ms timeout is plenty.
 */
void link_flush(void)
{
    uart_wait_tx_done(LINK_UART_PORT_NUM, TIMEOUT);
}
