// Firmware updates over Wi-Fi, for when the vision box is sealed. See wifi_ota.c for the
// whole procedure; components/wifi_ota/ota_upload.ps1 is the laptop side.
#pragma once

// Call in app_main() right after power_init(), before vision_init().
//
// Deep-sleep wake (a crow visit): returns at once, Wi-Fi stays off.
// Any other boot (power-on, reset, crash, after an update): opens the update access point
// in a background task and returns at once; the vision pipeline runs alongside it.
// Boot loop (3 crashes in a row, each within a minute of boot): never returns. The board
// stays in the update window instead of running vision_init() into the same crash, so a
// fixed build can still be uploaded. The main ESP32 then falls back to weight only.
void wifi_ota_start(void);
