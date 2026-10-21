// Power: deep sleep in step with the main ESP32 (see power.h).
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "link.h"
#include "storage.h"
#include "power.h"

// The link's UART RX pin (LINK_RXD in link.c): keep the two in sync. Must be one of GPIO0-15
// (the P4's LP IOs, the only pins that can wake it from deep sleep).
#define POWER_WAKE_PIN          GPIO_NUM_5
#define POWER_WAKE_MODE         ESP_EXT1_WAKEUP_ANY_LOW   // UART line idles high; a start bit pulls it low

static const char *TAG = "power";

/* power_init
 * 1. esp_sleep_get_wakeup_cause() -> esp_sleep_wakeup_cause_t:
 *      ESP_SLEEP_WAKEUP_EXT1       the PIR woke us (a crow is arriving)
 *      ESP_SLEEP_WAKEUP_UNDEFINED  not from sleep: power-on, reset, or a crash reboot
 *    ESP_LOGI which one. (Optional: esp_sleep_get_ext1_wakeup_status() is a bit mask of
 *    which pin(s) did it; with one wake pin it just confirms POWER_WAKE_PIN.)
 * 2. Nothing to arm here: the wake source only matters at the moment you sleep, and
 *    power_enter_deep_sleep() sets it up then.
 */
void power_init(void)
{
    esp_sleep_wakeup_cause_t wake_cause = esp_sleep_get_wakeup_cause();
    ESP_LOGI(TAG, "Woke from sleep caused by: %d", wake_cause);
    rtc_gpio_deinit(POWER_WAKE_PIN);
}

/* power_enter_deep_sleep (doesn't return; waking is a reset -> app_main() from the top)
 * 1. link_flush(): wait until the OK reply has physically left the UART. Deep sleep
 *    powers the UART down, and anything still in its FIFO is lost.
 * 2. storage_deinit(): unmount the SD card (esp_vfs_fat_sdcard_unmount), so FAT's
 *    cached sectors and directory updates are on the card before power drops.
 * 3. Wake pin's level while asleep: rtc_gpio_pullup_dis(POWER_WAKE_PIN) and
 *    rtc_gpio_pulldown_en(POWER_WAKE_PIN). (Why the pull-down? Your question to answer.)
 * 4. esp_sleep_enable_ext1_wakeup_io(1ULL << POWER_WAKE_PIN, POWER_WAKE_MODE)
 *    1ULL, not 1: the mask is 64-bit, one bit per GPIO.
 * 5. ESP_LOGI that you're going down (it prints before the console dies, if you
 *    want it visible add a short vTaskDelay or uart_wait_tx_done on the console too).
 * 6. esp_deep_sleep_start()
 */
void power_enter_deep_sleep(void)
{
    link_flush();
    storage_deinit();
    esp_sleep_enable_ext1_wakeup_io(1ULL << POWER_WAKE_PIN, POWER_WAKE_MODE);
    rtc_gpio_init(POWER_WAKE_PIN);
    rtc_gpio_pullup_en(POWER_WAKE_PIN);
    rtc_gpio_pulldown_dis(POWER_WAKE_PIN);
    ESP_LOGI(TAG, "entering deep sleep");
    esp_deep_sleep_start();
}
