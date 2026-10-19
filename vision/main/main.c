#include <stdio.h>
#include "link.h"
#include "vision.h"

void app_main(void)
{
    vision_init();      // camera + SD first, so READY means ready
    link_init();        // creates the job queue, sends READY
    vision_start();     // worker reads that queue
}
