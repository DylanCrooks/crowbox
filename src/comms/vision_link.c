#include "vision_link.h"
#include "pinout.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

#define TAG "vision_link"

#define VISION_UART_PORT        UART_NUM_2
#define VISION_UART_BAUD_RATE   115200      // must match the vision board
#define VISION_RX_BUF_SIZE      1024        // driver ring buffer; must be > the 128-byte HW FIFO
#define VISION_BYTE_TIMEOUT_MS  20          // gap that abandons a half-received frame

static bool s_link_up = false;
static uint8_t s_next_id = 0;

void vision_link_init(void) {
    uart_config_t cfg = {
        .baud_rate = VISION_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    esp_err_t err = uart_driver_install(VISION_UART_PORT, VISION_RX_BUF_SIZE, 0, 0, NULL, 0);
    if (err == ESP_OK) err = uart_param_config(VISION_UART_PORT, &cfg);
    if (err == ESP_OK) err = uart_set_pin(VISION_UART_PORT, PIN_UART_TX, PIN_UART_RX,
                                          UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART init failed (%s), vision disabled", esp_err_to_name(err));
        return;
    }
    s_link_up = true;

    // Wake the vision board: it sleeps with EXT1 armed on its RX pin, and this frame's first
    // start bit pulls that line low. The frame itself is lost while the P4 boots; it
    // announces itself with READY ~1-2 s later.
    vision_link_send(MSG_PING);
}

// ID 0 is reserved for the vision board's unprompted READY.
static uint8_t next_id(void) {
    s_next_id++;
    if (s_next_id == 0) s_next_id = 1;
    return s_next_id;
}

static void send_frame(uint8_t type, uint8_t id) {
    UART_msg_t m = { .type = type, .ID = id, .arg0 = 0, .arg1 = 0 };
    uint8_t out[MSG_FRAME_LEN];
    msg_pack(&m, out);
    uart_write_bytes(VISION_UART_PORT, out, MSG_FRAME_LEN);
}

// Reads bytes until one valid frame arrives or the deadline passes.
// A bad frame is resynced from the next START_BYTE inside it.
static bool read_frame(UART_msg_t *m, TickType_t deadline) {
    uint8_t in[MSG_FRAME_LEN];
    uint8_t count = 0;
    while (1) {
        TickType_t now = xTaskGetTickCount();
        if ((int32_t)(deadline - now) <= 0) return false;

        TickType_t wait = deadline - now;
        if (count > 0 && wait > pdMS_TO_TICKS(VISION_BYTE_TIMEOUT_MS)) {
            wait = pdMS_TO_TICKS(VISION_BYTE_TIMEOUT_MS);
        }

        uint8_t b;
        int len = uart_read_bytes(VISION_UART_PORT, &b, 1, wait);
        if (len <= 0) {
            count = 0;      // timed out mid-frame: sender stopped, drop the partial
            continue;
        }

        if (count == 0 && b != START_BYTE) continue;
        in[count++] = b;
        if (count < MSG_FRAME_LEN) continue;

        if (msg_unpack(in, m)) return true;

        ESP_LOGW(TAG, "bad frame, resyncing");
        uint8_t i = 1;
        while (i < MSG_FRAME_LEN && in[i] != START_BYTE) i++;
        count = MSG_FRAME_LEN - i;
        memmove(in, &in[i], count);
    }
}

bool vision_link_request(uint8_t type, UART_msg_t *reply, uint32_t timeout_ms) {
    if (!s_link_up) return false;

    uint8_t id = next_id();
    uart_flush_input(VISION_UART_PORT);     // drop stale replies and READYs from earlier
    send_frame(type, id);

    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    UART_msg_t m;
    while (read_frame(&m, deadline)) {
        if (m.ID != id) {
            ESP_LOGD(TAG, "ignoring type=%u id=%u (waiting for id=%u)", m.type, m.ID, id);
            continue;
        }
        if (m.type == MSG_QUEUED) continue;     // accepted; final reply follows
        *reply = m;
        return true;
    }
    ESP_LOGW(TAG, "request type=%u id=%u timed out", type, id);
    return false;
}

void vision_link_send(uint8_t type) {
    if (!s_link_up) return;
    uart_flush_input(VISION_UART_PORT);     // unread replies would otherwise pile up in the ring buffer
    send_frame(type, next_id());
}
