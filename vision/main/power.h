// Power: deep sleep in step with the main ESP32.
// Sleep: the main ESP32 sends MSG_SLEEP right before its own deep sleep.
// Wake:  the PIR output, jumpered to POWER_WAKE_PIN inside the vision box, wakes both boards
//        on the same motion (the main ESP32 wakes from the same signal on its GPIO34).
#pragma once

// Call first thing in app_main(): logs why the chip woke, sets up the wake pin.
void power_init(void);

// Flushes/unmounts the SD card, arms the PIR wake, enters deep sleep. Does not return:
// waking from deep sleep is a reset, so app_main() runs again from the top.
void power_enter_deep_sleep(void);
