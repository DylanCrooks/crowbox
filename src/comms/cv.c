#include "cv.h"
#include "config.h"
#include "vision_link.h"
#include "mcp23017.h"
#include "pinout.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stdint.h>

#define TAG "cv"

#define DEPOSIT_MIN_WEIGHT_G        2       // noise floor: anything at or below isn't a deposit
#define CV_STRICTNESS_LEVEL         3       // stage1_validation_strictness at which vision gets a veto
#define VISION_CLASSIFY_TIMEOUT_MS  1500    // capture + SD write + inference, with margin
#define VISION_BUSY_RETRY_MS        300
#define VISION_MIN_CONFIDENCE       60      // 0-99
#define VISION_REF_TIMEOUT_MS       1500    // capture + grayscale, with margin
#define VISION_SLEEP_TIMEOUT_MS     1500    // P4 may first finish saving the reference photo

void cv_light(bool on) {
    mcp23017_set_pin(0, MCP_BIT_COB_LED, on);
}

void cv_capture_reference(void) {
    UART_msg_t reply;
    bool got = vision_link_request(MSG_CAPTURE_REF, &reply, VISION_REF_TIMEOUT_MS);
    if (!got || reply.type != MSG_OK) {
        ESP_LOGW(TAG, "reference capture failed (%s)", !got ? "timeout" : "error");
    }
}

void cv_vision_sleep(void) {
    UART_msg_t reply;
    bool got = vision_link_request(MSG_SLEEP, &reply, VISION_SLEEP_TIMEOUT_MS);
    if (!got || reply.type != MSG_OK) {
        // Not fatal: the P4 just stays awake (costs power) until the next visit's sleep.
        ESP_LOGW(TAG, "vision board didn't confirm sleep (%s)", !got ? "timeout" : "error");
    }
}

bool classify_object(int32_t weight) {
    if (weight <= DEPOSIT_MIN_WEIGHT_G) return false;

    // Below the CV level, weight decides. Still ask the vision board to classify
    // (it saves the photo: training data), but don't wait, so the reward isn't delayed.
    if (g_config.stage1_validation_strictness < CV_STRICTNESS_LEVEL) {
        vision_link_send(MSG_CLASSIFY);
        return true;
    }

    UART_msg_t reply;
    bool got = vision_link_request(MSG_CLASSIFY, &reply, VISION_CLASSIFY_TIMEOUT_MS);
    if (got && reply.type == MSG_ERROR && reply.arg0 == ERR_BUSY) {
        vTaskDelay(pdMS_TO_TICKS(VISION_BUSY_RETRY_MS));
        got = vision_link_request(MSG_CLASSIFY, &reply, VISION_CLASSIFY_TIMEOUT_MS);
    }

    if (!got || reply.type != MSG_RESULT) {
        ESP_LOGW(TAG, "vision unavailable (%s), weight-only fallback",
                 !got ? "timeout" : "error");
        return true;
    }

    bool valid = (reply.arg0 == CLASS_COIN) && (reply.arg1 >= VISION_MIN_CONFIDENCE);
    ESP_LOGI(TAG, "vision: class=%u conf=%u -> %s", reply.arg0, reply.arg1,
             valid ? "valid" : "invalid");
    return valid;
}
