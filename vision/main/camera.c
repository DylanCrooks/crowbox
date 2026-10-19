// Camera: open the OV5647 via esp-video, configure the mode, discard settling frames, capture.
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "esp_log.h"
#include "esp_video_init.h"
#include "esp_video_device.h"
#include "linux/videodev2.h"
#include "camera.h"

// SCCB = the camera's I2C control bus. Placeholder: GPIO7/8 is the board's
// internal I2C bus; confirm the camera connector uses it when the board arrives.
#define CAMERA_SCCB_I2C_PORT    0
#define CAMERA_SCCB_SDA_PIN     7
#define CAMERA_SCCB_SCL_PIN     8
#define CAMERA_SCCB_FREQ_HZ     100000
#define CAMERA_RESET_PIN        -1      // Pi-style module: no reset/powerdown lines wired
#define CAMERA_PWDN_PIN         -1

#define CAMERA_BUFFER_COUNT     2
#define CAMERA_WARMUP_FRAMES    15      // let auto-exposure / white balance settle; tune on the board

static const char *TAG = "camera";

static int s_fd = -1;
static uint8_t *s_buf[CAMERA_BUFFER_COUNT];
static size_t s_buf_len[CAMERA_BUFFER_COUNT];


esp_err_t camera_init(void) {
    esp_video_init_csi_config_t csi = {
        .sccb_config = {
            .init_sccb = true,
            .i2c_config = {
                .port = CAMERA_SCCB_I2C_PORT,
                .scl_pin = CAMERA_SCCB_SCL_PIN,
                .sda_pin = CAMERA_SCCB_SDA_PIN
            },
            .freq = CAMERA_SCCB_FREQ_HZ,
        },
        .reset_pin = CAMERA_RESET_PIN,
        .pwdn_pin = CAMERA_PWDN_PIN,
    };
    esp_video_init_config_t cfg = {.csi = &csi};
    esp_err_t init_check = esp_video_init(&cfg);
    if (init_check != ESP_OK) {
        ESP_LOGE(TAG, "esp_video_init failed: %s", esp_err_to_name(init_check));
        return init_check;
    }
    s_fd = open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, O_RDONLY);
    if (s_fd < 0) {
        ESP_LOGE(TAG, "failed to open %s", ESP_VIDEO_MIPI_CSI_DEVICE_NAME);
        return ESP_FAIL;
    }
    struct v4l2_format fmt = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .fmt.pix.width = CAMERA_WIDTH,
        .fmt.pix.height = CAMERA_HEIGHT,
        .fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565,
    };
    int ioctl_check = ioctl(s_fd, VIDIOC_S_FMT, &fmt);
    if (ioctl_check != 0) {
        ESP_LOGE(TAG, "VIDIOC_S_FMT failed");
        return ESP_FAIL;
    }
    struct v4l2_requestbuffers reqbufs = {
        .count = CAMERA_BUFFER_COUNT,
	    .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,		/* enum v4l2_buf_type */
		.memory = V4L2_MEMORY_MMAP		/* enum v4l2_memory */
            /*  .capabilities;
            __u8			flags;
            __u8			reserved[3];
            */
    };

    

    int reqbufs_check = ioctl(s_fd, VIDIOC_REQBUFS, &reqbufs);
    if (reqbufs_check != 0) {
        ESP_LOGE(TAG, "VIDIOC_REQBUFS failed");
        return ESP_FAIL;
    }
    if (reqbufs.count != CAMERA_BUFFER_COUNT) {
        ESP_LOGE(TAG, "VIDIOC_REQBUFS returned less (or more?) than CAMERA_BUFFER_COUNT buffers");
        return ESP_FAIL;
    }
    for (int i = 0; i < CAMERA_BUFFER_COUNT; i++) {
        struct v4l2_buffer querybuf = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
        .index = i
        };
        int querybuf_check = ioctl(s_fd, VIDIOC_QUERYBUF, &querybuf);
        if (querybuf_check != 0) {
        ESP_LOGE(TAG, "VIDIOC_QUERYBUF failed");
        return ESP_FAIL; 
        }
        void* bufptr;
        bufptr = mmap(NULL, querybuf.length, PROT_READ | PROT_WRITE, MAP_SHARED, s_fd, querybuf.m.offset);
        if (bufptr == MAP_FAILED) {
            ESP_LOGE(TAG, "mmap of buffer %d failed", i);
            return ESP_FAIL;
        }
        s_buf[i] = bufptr;
        s_buf_len[i] = querybuf.length;
        
    }

    
    return ESP_OK;
}

esp_err_t camera_capture(uint8_t *out, size_t out_size, size_t *len) {
    if (out_size < CAMERA_FRAME_BYTES) {return ESP_FAIL;}

    for (int i = 0; i < CAMERA_BUFFER_COUNT; i++) {
        struct v4l2_buffer buffer = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
        .index = i
        };
        if (ioctl(s_fd, VIDIOC_QBUF, &buffer) != 0) {
            ESP_LOGE(TAG, "VIDIOC_QBUF failed");
            return ESP_FAIL;
        }
    }
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(s_fd, VIDIOC_STREAMON, &type) != 0) {
        ESP_LOGE(TAG, "VIDIOC_STREAMON failed");
        return ESP_FAIL;
    }
    struct v4l2_buffer b = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
        };
    for (int i = 0; i < CAMERA_WARMUP_FRAMES; i++) {
        if (ioctl(s_fd, VIDIOC_DQBUF, &b) != 0) {goto cleanup;}
        if (ioctl(s_fd, VIDIOC_QBUF, &b) != 0) {goto cleanup;}
    }
    if (ioctl(s_fd, VIDIOC_DQBUF, &b) != 0) {goto cleanup;}
    if (!(b.flags & V4L2_BUF_FLAG_DONE) || (b.flags & V4L2_BUF_FLAG_ERROR)) {
        ESP_LOGE(TAG, "bad frame (flags 0x%lx)", (unsigned long)b.flags);
        goto cleanup;
    }
    memcpy(out, s_buf[b.index], b.bytesused);
    *len = b.bytesused;
    ioctl(s_fd, VIDIOC_STREAMOFF, &type);

    return ESP_OK;


    cleanup:
        ioctl(s_fd, VIDIOC_STREAMOFF, &type);
        return ESP_FAIL;
    

}





