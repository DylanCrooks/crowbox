#include "sleep.h"
#include "esp_sleep.h"
#include "pinout.h"

// sleep_enter_deep
// Call esp_sleep_enable_ext0_wakeup with PIN_PIR cast to gpio_num_t, level = 1 (wake on HIGH).
// Then call esp_deep_sleep_start(). Nothing after this line will execute.
void sleep_enter_deep(void) {
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_PIR, 1);
    esp_deep_sleep_start();
}
