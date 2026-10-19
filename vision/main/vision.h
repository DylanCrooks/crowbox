// Vision worker: runs CLASSIFY / CAPTURE_REF jobs handed over by the link task.
#pragma once

// Starts the camera and SD card and allocates the frame buffers. Call before link_init(),
// so the hardware is up before READY goes out. A failure here is logged; jobs then
// get ERR_CAMERA_FAILED / photos are skipped, rather than the board halting.
void vision_init(void);

// Starts the worker task. Call after link_init() (the worker reads its job queue).
void vision_start(void);
