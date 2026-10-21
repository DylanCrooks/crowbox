#include <stdio.h>
#include "link.h"
#include "power.h"
#include "vision.h"

void app_main(void)
{
    power_init();       // first: logs why we woke (PIR or power-on), arms the wake pin
    vision_init();      // camera + SD first, so READY means ready
    link_init();        // creates the job queue, sends READY
    vision_start();     // worker reads that queue
}
