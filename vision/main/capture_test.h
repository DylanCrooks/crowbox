// THROWAWAY bench test: takes photos without the main ESP32. Remove its two lines (main.c,
// main/CMakeLists.txt) when done.
#pragma once

// Call after vision_start(). Every CAPTURE_TEST_PERIOD_S: capture -> save JPEG to the SD card ->
// blob-classify (the first frame is the empty-pit reference) -> log the result, the timing,
// and which RGB565 byte order the frame looks like.
void capture_test_start(void);
