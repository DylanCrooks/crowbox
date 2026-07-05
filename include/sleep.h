#pragma once

// sleep_enter_deep
// Prepare for and enter deep sleep. Call this when the box is idle (no crow present,
// reward chamber restocked, trapdoor locked).
// Step 1: call esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_PIR, 1)
//         — configures GPIO34 HIGH as the wake trigger (PIR fires = wake up)
// Step 2: call esp_deep_sleep_start()
//         — this function never returns. ESP32 resets on wake, app_main runs again.
// NOTE: all peripheral state (LEDC, I2C, GPIO config) is lost on wake.
//       All init functions must be called again after every boot/wake.
void sleep_enter_deep(void);
