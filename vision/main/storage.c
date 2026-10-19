// microSD: mount, and save JPEGs safely (temp file, flush, fsync, close, rename).
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#include "driver/jpeg_encode.h"
#include "camera.h"
#include "storage.h"

// SD slot pins: the P4's dedicated SDMMC pins, which the P4-NANO's slot uses (GPIO39-44).
#define STORAGE_SD_CLK_PIN      43
#define STORAGE_SD_CMD_PIN      44
#define STORAGE_SD_D0_PIN       39
#define STORAGE_SD_D1_PIN       40
#define STORAGE_SD_D2_PIN       41
#define STORAGE_SD_D3_PIN       42
#define STORAGE_SD_BUS_WIDTH    4
// Card power. Espressif's dev board feeds it from the P4's on-chip LDO channel 4;
// docs/crowbox_vision_box.txt says the P4-NANO has a power switch on GPIO45.
// Confirm against the Waveshare schematic when the board arrives.
#define STORAGE_SD_LDO_CHAN     4

#define STORAGE_MAX_OPEN_FILES  4
#define STORAGE_TMP_PATH        STORAGE_MOUNT_POINT "/tmp.jpg"
#define STORAGE_PHOTO_FMT       STORAGE_MOUNT_POINT "/img%05lu.jpg"   // img00042.jpg
#define STORAGE_PATH_MAX        32

#define STORAGE_JPEG_QUALITY    80          // 1-100
#define STORAGE_JPEG_TIMEOUT_MS 200         // must exceed one frame's encode time
#define STORAGE_JPEG_OUT_MAX    (512 * 1024) // generous: a 1280x960 photo at q80 is ~100-300 KB

static const char *TAG = "storage";

static sdmmc_card_t *s_card;
static jpeg_encoder_handle_t s_jpeg;
static uint8_t *s_jpeg_out;     // compressed output, from jpeg_alloc_encoder_mem
static size_t s_jpeg_out_size;
static uint32_t s_next_photo_id;

esp_err_t storage_init(void)
{
    sd_pwr_ctrl_ldo_config_t sd_pwr_config = {.ldo_chan_id = STORAGE_SD_LDO_CHAN};
    sd_pwr_ctrl_handle_t sd_pwr_ctrl_handle = NULL;
    sd_pwr_ctrl_new_on_chip_ldo(&sd_pwr_config, &sd_pwr_ctrl_handle); //placeholder until we know if NANO
    // uses LDO or GPIO45 power switch
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.pwr_ctrl_handle = sd_pwr_ctrl_handle;

    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = STORAGE_SD_BUS_WIDTH;
    slot.clk = STORAGE_SD_CLK_PIN;
    slot.cmd = STORAGE_SD_CMD_PIN;
    slot.d0 = STORAGE_SD_D0_PIN;
    slot.d1 = STORAGE_SD_D1_PIN;
    slot.d2 = STORAGE_SD_D2_PIN;
    slot.d3 = STORAGE_SD_D3_PIN;

    esp_vfs_fat_sdmmc_mount_config_t mcfg = {
        .format_if_mount_failed = false,
        .max_files = STORAGE_MAX_OPEN_FILES,
        .allocation_unit_size = 16 * 1024
    };

    esp_err_t mount_check = esp_vfs_fat_sdmmc_mount(STORAGE_MOUNT_POINT, &host, &slot, &mcfg, &s_card);
    if (mount_check != 0) {
        ESP_LOGE(TAG,"shizzz....mount check failed.....shizzzzzzzzzzzz");
        return ESP_FAIL;}
    
    
    jpeg_encode_engine_cfg_t jecfg = {.timeout_ms = STORAGE_JPEG_TIMEOUT_MS};
    if (jpeg_new_encoder_engine(&jecfg, &s_jpeg) != ESP_OK) {goto cleanup;}
    jpeg_encode_memory_alloc_cfg_t jemacfg = {.buffer_direction = JPEG_ENC_ALLOC_OUTPUT_BUFFER};
    s_jpeg_out = jpeg_alloc_encoder_mem(STORAGE_JPEG_OUT_MAX, &jemacfg, &s_jpeg_out_size);
    if (s_jpeg_out == NULL) {goto cleanup;}

    DIR *dir = opendir(STORAGE_MOUNT_POINT);
    if (dir == NULL) {goto cleanup;}
    struct dirent *e = NULL;
    while ((e = readdir(dir)) != NULL){
        unsigned long n;
        int sscanf_res = sscanf(e->d_name, "img%5lu", &n);
        if (sscanf_res == 1 && n >= s_next_photo_id) {
            s_next_photo_id = n + 1;
        }

    }
    closedir(dir);
    unlink(STORAGE_TMP_PATH);



    return ESP_OK; 

    cleanup:

    ESP_LOGE(TAG, "JPEG encoder setup or photo scan failed");
    return ESP_FAIL;
}

esp_err_t storage_save_frame(const uint8_t *rgb565, size_t len, uint32_t *photo_id)
{
    uint32_t jpeg_size;
    jpeg_encode_cfg_t jencfg = {
        .width = CAMERA_WIDTH,
        .height = CAMERA_HEIGHT,
        .src_type = JPEG_ENCODE_IN_FORMAT_RGB565,
        .sub_sample = JPEG_DOWN_SAMPLING_YUV420,
        .image_quality = STORAGE_JPEG_QUALITY
    };
    if (jpeg_encoder_process(s_jpeg, &jencfg, rgb565, len, s_jpeg_out, s_jpeg_out_size, &jpeg_size) != ESP_OK) {
        ESP_LOGE(TAG, "JPEG encode failed");
        return ESP_FAIL;}

    FILE *file = fopen(STORAGE_TMP_PATH, "wb");
    if (file == NULL) {
        ESP_LOGE(TAG,"fopen returned null, storage.c");
        return ESP_FAIL;}
    uint32_t fwrite_res = fwrite(s_jpeg_out, 1, jpeg_size, file);
    if (jpeg_size != fwrite_res) {goto cleanup;}

    if (fflush(file) != 0) {goto cleanup;}
    if (fsync(fileno(file)) != 0) {goto cleanup;}
    fclose(file);

    char path[STORAGE_PATH_MAX];
    snprintf(path, sizeof(path), STORAGE_PHOTO_FMT, (unsigned long)s_next_photo_id);
    if (rename(STORAGE_TMP_PATH, path) != 0) {
        ESP_LOGE(TAG, "rename to %s failed", path);
        unlink(STORAGE_TMP_PATH);
        return ESP_FAIL;}

    if (photo_id != NULL) {*photo_id = s_next_photo_id;}
    s_next_photo_id++;
    ESP_LOGI(TAG, "saved %s (%lu bytes)", path, (unsigned long)jpeg_size);
    return ESP_OK;

    cleanup:
    fclose(file);
    unlink(STORAGE_TMP_PATH);
    return ESP_FAIL;
}
