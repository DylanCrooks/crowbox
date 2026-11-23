#include <stdio.h>
#include "link.h"
#include "power.h"
#include "vision.h"
#include "wifi_ota.h"
//#include "capture_test.h"   // bench capture test: uncomment with the two lines below

void app_main(void)
{
    power_init();       // first: logs why we woke (PIR or power-on), arms the wake pin
    wifi_ota_start();   // Wi-Fi update window (not on deep-sleep wakes); see components/wifi_ota
    vision_init();      // camera + SD first, so READY means ready
    link_init();        // creates the job queue, sends READY
    vision_start();     // worker reads that queue
    //capture_test_start();   // bench capture test (capture_test.c): also uncomment it in main/CMakeLists.txt
}
