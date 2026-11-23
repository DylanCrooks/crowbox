// Firmware updates over Wi-Fi, for when the vision box is sealed.
//
// The P4 has no radio: Wi-Fi runs on the board's ESP32-C6, which esp_hosted drives over SDIO.
// esp_wifi_remote gives the P4 the normal esp_wifi API on top of that.
//
// Procedure
//   1. Power-cycle the box. On any boot that isn't a deep-sleep wake, the board opens an access
//      point (WIFI_OTA_AP_SSID, password in wifi_ota_secrets.h) for WIFI_OTA_WINDOW_S seconds,
//      longer while a laptop stays connected. Deep-sleep wakes (crow visits) never turn Wi-Fi on.
//   2. Join it from the laptop and run ota_upload.ps1. It POSTs build/vision.bin to
//      http://192.168.4.1/update. The image goes into the app slot that isn't running, ESP-IDF
//      verifies it (checksum, SHA-256, chip and chip revision), and the board reboots into it.
//   3. The new app boots "pending verify" and opens the access point again. The first HTTP
//      request that reaches it (ota_upload.ps1 polls GET /status) confirms it: proof that the
//      new app can take the next update. If nothing confirms it within
//      WIFI_OTA_CONFIRM_TIMEOUT_S, or it crashes or resets first, the bootloader boots the
//      previous app instead.
//
// Safe mode: three crash reboots in a row, each within WIFI_OTA_STABLE_S of boot, means the
// app is crashing on every boot. wifi_ota_start() then doesn't return, so the crash isn't
// reached, and the update window stays open (up to WIFI_OTA_MAX_S, then one retry of a normal
// boot).
//
// The vision pipeline keeps running during a normal window; nothing here touches it. Don't
// update in the middle of a crow visit: the reboot cuts off whatever photo is being saved.
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_ota_ops.h"
#include "esp_app_desc.h"
#include "esp_app_format.h"
#include "esp_idf_version.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "esp_hosted.h"
#include "esp_hosted_event.h"
#include "wifi_ota_secrets.h"
#include "wifi_ota.h"

#define WIFI_OTA_WINDOW_S           180         // AP stays up at least this long after boot
#define WIFI_OTA_IDLE_S             120         // ...and until this long after a laptop was last seen
#define WIFI_OTA_MAX_S              (30 * 60)   // hard cap: a forgotten laptop can't keep Wi-Fi on all day
#define WIFI_OTA_CONFIRM_TIMEOUT_S  (5 * 60)    // a new app must be reached over HTTP within this
#define WIFI_OTA_STABLE_S           60          // a crash later than this after boot isn't a boot loop
#define WIFI_OTA_SAFE_MODE_CRASHES  3

#define WIFI_OTA_AP_CHANNEL         6
#define WIFI_OTA_AP_MAX_STATIONS    2
#define WIFI_OTA_TASK_STACK         4096
#define WIFI_OTA_TASK_PRIORITY      3           // below the vision worker (5) and the link task (10)
#define WIFI_OTA_HTTPD_STACK        8192
#define WIFI_OTA_CHUNK              4096        // upload buffer
#define WIFI_OTA_RECV_RETRIES       3           // socket timeouts tolerated per chunk
#define WIFI_OTA_REBOOT_DELAY_MS    1000        // lets the HTTP reply reach the laptop first
#define WIFI_OTA_CRASH_MAGIC        0xC20B0C5Eu

_Static_assert(sizeof(WIFI_OTA_AP_PASSWORD) - 1 >= 8 && sizeof(WIFI_OTA_AP_PASSWORD) - 1 <= 63,
               "WIFI_OTA_AP_PASSWORD must be 8-63 characters (WPA2)");

static const char *TAG = "wifi_ota";

// Survive panics and software resets (not power loss): counts back-to-back crash boots.
RTC_NOINIT_ATTR static uint32_t s_crash_magic;
RTC_NOINIT_ATTR static uint32_t s_crash_count;

static bool s_safe_mode;
static volatile bool s_pending;             // running app is new and not yet confirmed
static volatile bool s_updating;            // an upload is being written
static volatile bool s_transport_failed;    // the C6 link died
static volatile int s_stations;             // laptops joined to the AP
static volatile int64_t s_last_activity_us;

static esp_timer_handle_t s_rollback_timer;
static esp_timer_handle_t s_reboot_timer;
static httpd_handle_t s_httpd;
static esp_hosted_coprocessor_fwver_t s_c6_ver;
static bool s_have_c6_ver;

static void note_activity(void)
{
    s_last_activity_us = esp_timer_get_time();
}

// ---------------------------------------------------------------- confirm / rollback / reboot

static void rollback_timeout_cb(void *arg)
{
    ESP_LOGE(TAG, "new firmware wasn't confirmed within %d s: rolling back", WIFI_OTA_CONFIRM_TIMEOUT_S);
    esp_ota_mark_app_invalid_rollback_and_reboot();     // doesn't return if a previous app exists
}

static void roll_back_now(const char *why)
{
    ESP_LOGE(TAG, "%s: rolling back to the previous firmware", why);
    esp_ota_mark_app_invalid_rollback_and_reboot();
}

// Any request that reaches us proves the new app can be updated again.
static void confirm_running_app(void)
{
    if (!s_pending) {
        return;
    }
    if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
        s_pending = false;
        esp_timer_stop(s_rollback_timer);
        ESP_LOGI(TAG, "new firmware confirmed");
    }
}

static void reboot_cb(void *arg)
{
    esp_restart();
}

static void stable_cb(void *arg)
{
    s_crash_count = 0;
}

// ---------------------------------------------------------------- HTTP

static const char *reset_reason_str(esp_reset_reason_t r)
{
    switch (r) {
    case ESP_RST_POWERON:   return "power-on";
    case ESP_RST_EXT:       return "reset pin";
    case ESP_RST_SW:        return "software restart (e.g. after an update)";
    case ESP_RST_PANIC:     return "crash (panic)";
    case ESP_RST_INT_WDT:   return "crash (interrupt watchdog)";
    case ESP_RST_TASK_WDT:  return "crash (task watchdog)";
    case ESP_RST_WDT:       return "crash (watchdog)";
    case ESP_RST_DEEPSLEEP: return "deep-sleep wake";
    case ESP_RST_BROWNOUT:  return "brownout";
    default:                return "other";
    }
}

// GET / and GET /status: what's running, and confirms a pending update.
static esp_err_t status_get(httpd_req_t *req)
{
    note_activity();
    bool was_pending = s_pending;
    confirm_running_app();

    const esp_app_desc_t *app = esp_app_get_description();
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *other = esp_ota_get_next_update_partition(NULL);
    esp_app_desc_t other_desc;
    bool have_other = other != NULL && esp_ota_get_partition_description(other, &other_desc) == ESP_OK;

    char c6[48] = "unknown";
    if (s_have_c6_ver) {
        snprintf(c6, sizeof(c6), "%" PRIu32 ".%" PRIu32 ".%" PRIu32,
                 s_c6_ver.major1, s_c6_ver.minor1, s_c6_ver.patch1);
    }

    const size_t cap = 1024;
    char *out = malloc(cap);
    if (out == NULL) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "out of memory");
    }
    int n = snprintf(out, cap,
        "CrowBox vision board\n"
        "firmware:   %s (built %s %s, ESP-IDF %s)\n"
        "running:    %s%s%s\n"
        "other slot: %s: %s\n"
        "last reset: %s\n"
        "uptime:     %lld s\n"
        "C6 Wi-Fi:   esp_hosted %s on the C6, %d.%d.%d in this build\n"
        "free heap:  %u KB internal, %u KB PSRAM\n",
        app->version, app->date, app->time, app->idf_ver,
        running->label,
        was_pending ? " (new firmware: confirmed by this request)" : "",
        s_safe_mode ? " [SAFE MODE: vision is not running]" : "",
        other ? other->label : "-", have_other ? other_desc.version : "(empty)",
        reset_reason_str(esp_reset_reason()),
        (long long)(esp_timer_get_time() / 1000000),
        c6, ESP_HOSTED_VERSION_MAJOR_1, ESP_HOSTED_VERSION_MINOR_1, ESP_HOSTED_VERSION_PATCH_1,
        (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
        (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
    if (n >= (int)cap) {
        n = cap - 1;
    }
    httpd_resp_set_type(req, "text/plain");
    esp_err_t err = httpd_resp_send(req, out, n);
    free(out);
    return err;
}

// Receives up to len bytes, riding out a few socket timeouts. Returns bytes read, or -1.
static int recv_some(httpd_req_t *req, char *buf, size_t len)
{
    for (int tries = 0; tries < WIFI_OTA_RECV_RETRIES; tries++) {
        int n = httpd_req_recv(req, buf, len);
        if (n > 0) {
            return n;
        }
        if (n != HTTPD_SOCK_ERR_TIMEOUT) {
            return -1;      // 0 = connection closed early; < 0 = socket error
        }
    }
    return -1;
}

static esp_err_t send_error(httpd_req_t *req, const char *status, const char *msg)
{
    ESP_LOGE(TAG, "update rejected: %s", msg);
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, msg);
    return ESP_OK;      // the error went out as a reply; keep the server running
}

// POST /update, body = the raw app image (build/vision.bin).
static esp_err_t update_post(httpd_req_t *req)
{
    note_activity();
    confirm_running_app();      // a pending app can't start an update (ESP-IDF refuses)

    if (s_updating) {
        return send_error(req, "409 Conflict", "an update is already in progress\n");
    }
    const esp_partition_t *target = esp_ota_get_next_update_partition(NULL);
    if (target == NULL) {
        return send_error(req, "500 Internal Server Error", "no OTA slot (partition table?)\n");
    }
    // The image header and app description sit in the first few hundred bytes.
    const size_t desc_off = sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t);
    const size_t head_len = desc_off + sizeof(esp_app_desc_t);
    if (req->content_len < head_len || req->content_len > target->size) {
        return send_error(req, "400 Bad Request", "body must be the app image (build/vision.bin)\n");
    }

    char *buf = malloc(WIFI_OTA_CHUNK);
    if (buf == NULL) {
        return send_error(req, "500 Internal Server Error", "out of memory\n");
    }
    s_updating = true;
    esp_ota_handle_t ota = 0;
    bool begun = false;
    const char *status = "500 Internal Server Error";
    const char *msg = "upload interrupted\n";

    // Read the head first and check it's a vision build for this chip before erasing anything.
    size_t have = 0;
    while (have < head_len) {
        int n = recv_some(req, buf + have, WIFI_OTA_CHUNK - have);
        if (n < 0) {
            goto fail;
        }
        have += n;
    }
    const esp_image_header_t *hdr = (const esp_image_header_t *)buf;
    const esp_app_desc_t *desc = (const esp_app_desc_t *)(buf + desc_off);
    if (hdr->magic != ESP_IMAGE_HEADER_MAGIC || desc->magic_word != ESP_APP_DESC_MAGIC_WORD) {
        status = "400 Bad Request";
        msg = "not an app image: send build/vision.bin (not the bootloader or a merged .bin)\n";
        goto fail;
    }
    if (hdr->chip_id != CONFIG_IDF_FIRMWARE_CHIP_ID) {
        status = "400 Bad Request";
        msg = "image is built for a different chip\n";
        goto fail;
    }
    if (strncmp(desc->project_name, esp_app_get_description()->project_name,
                sizeof(desc->project_name)) != 0) {
        status = "400 Bad Request";
        msg = "image isn't the vision firmware (wrong project)\n";
        goto fail;
    }
    // Copy the version now: buf is overwritten by the next chunk, then freed.
    char new_version[sizeof(desc->version) + 1];
    strlcpy(new_version, desc->version, sizeof(new_version));
    ESP_LOGI(TAG, "receiving %s (%u bytes) into %s", new_version, (unsigned)req->content_len,
             target->label);

    // Sequential writes: erase each sector as it's reached, not the whole 6 MB slot up front
    // (that takes long enough to time out the upload).
    esp_err_t err = esp_ota_begin(target, OTA_WITH_SEQUENTIAL_WRITES, &ota);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin: %s", esp_err_to_name(err));
        msg = "couldn't start writing the OTA slot\n";
        goto fail;
    }
    begun = true;

    size_t received = 0;
    while (true) {
        if (esp_ota_write(ota, buf, have) != ESP_OK) {
            msg = "flash write failed\n";
            goto fail;
        }
        received += have;
        note_activity();
        if (received >= req->content_len) {
            break;
        }
        int n = recv_some(req, buf, WIFI_OTA_CHUNK);
        if (n < 0) {
            goto fail;
        }
        have = n;
    }

    // Verifies the whole image: checksum, SHA-256, chip ID and chip revision range.
    err = esp_ota_end(ota);
    begun = false;
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end: %s", esp_err_to_name(err));
        status = "400 Bad Request";
        msg = (err == ESP_ERR_OTA_VALIDATE_FAILED)
              ? "image failed verification (corrupt upload, or built for another chip revision)\n"
              : "couldn't finish the OTA write\n";
        goto fail;
    }
    err = esp_ota_set_boot_partition(target);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition: %s", esp_err_to_name(err));
        msg = "couldn't switch the boot slot\n";
        goto fail;
    }
    free(buf);

    char reply[192];
    snprintf(reply, sizeof(reply),
             "OK: wrote %s to %s, rebooting into it. Reach /status within %d s to confirm it, "
             "or it rolls back.\n", new_version, target->label, WIFI_OTA_CONFIRM_TIMEOUT_S);
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, reply);
    ESP_LOGI(TAG, "update written; rebooting");
    esp_timer_start_once(s_reboot_timer, WIFI_OTA_REBOOT_DELAY_MS * 1000);
    return ESP_OK;      // s_updating stays set: nothing else starts before the reboot

fail:
    if (begun) {
        esp_ota_abort(ota);
    }
    free(buf);
    s_updating = false;
    return send_error(req, status, msg);
}

static bool start_http(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.stack_size = WIFI_OTA_HTTPD_STACK;
    cfg.task_priority = WIFI_OTA_TASK_PRIORITY;
    cfg.recv_wait_timeout = 10;
    cfg.send_wait_timeout = 10;
    cfg.max_open_sockets = 3;
    cfg.lru_purge_enable = true;

    if (httpd_start(&s_httpd, &cfg) != ESP_OK) {
        return false;
    }
    static const httpd_uri_t root = { .uri = "/", .method = HTTP_GET, .handler = status_get };
    static const httpd_uri_t status = { .uri = "/status", .method = HTTP_GET, .handler = status_get };
    static const httpd_uri_t update = { .uri = "/update", .method = HTTP_POST, .handler = update_post };
    httpd_register_uri_handler(s_httpd, &root);
    httpd_register_uri_handler(s_httpd, &status);
    httpd_register_uri_handler(s_httpd, &update);
    return true;
}

// ---------------------------------------------------------------- Wi-Fi

static void on_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STACONNECTED) {
        s_stations++;
        note_activity();
        ESP_LOGI(TAG, "laptop joined the update AP");
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STADISCONNECTED) {
        if (s_stations > 0) {
            s_stations--;
        }
        note_activity();
    } else if (base == ESP_HOSTED_EVENT && id == ESP_HOSTED_EVENT_TRANSPORT_FAILURE) {
        s_transport_failed = true;
    }
}

static bool start_wifi(void)
{
    // The C6 keeps its own Wi-Fi settings; NVS here is only for esp_wifi's host side.
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_flash_init: %s (continuing)", esp_err_to_name(err));
    }
    if (esp_netif_init() != ESP_OK) {
        return false;
    }
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {    // INVALID_STATE: already exists
        return false;
    }
    esp_event_handler_register(ESP_HOSTED_EVENT, ESP_EVENT_ANY_ID, on_event, NULL);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_event, NULL);
    esp_netif_create_default_wifi_ap();     // 192.168.4.1, with a DHCP server for the laptop

    // Resets the C6 and brings up the SDIO link to it, then initializes Wi-Fi there.
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&init);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init: %s (is the C6 running esp_hosted slave firmware?)",
                 esp_err_to_name(err));
        return false;
    }

    s_have_c6_ver = (esp_hosted_get_coprocessor_fwversion(&s_c6_ver) == ESP_OK);
    if (!s_have_c6_ver) {
        ESP_LOGW(TAG, "couldn't read the C6's esp_hosted version");
    } else if (s_c6_ver.major1 != ESP_HOSTED_VERSION_MAJOR_1 || s_c6_ver.minor1 != ESP_HOSTED_VERSION_MINOR_1) {
        ESP_LOGW(TAG, "C6 runs esp_hosted %" PRIu32 ".%" PRIu32 ".%" PRIu32 ", this build is %d.%d.%d: "
                 "versions should match (update the C6 on the bench)",
                 s_c6_ver.major1, s_c6_ver.minor1, s_c6_ver.patch1,
                 ESP_HOSTED_VERSION_MAJOR_1, ESP_HOSTED_VERSION_MINOR_1, ESP_HOSTED_VERSION_PATCH_1);
    } else {
        ESP_LOGI(TAG, "C6 esp_hosted %" PRIu32 ".%" PRIu32 ".%" PRIu32 " (matches this build)",
                 s_c6_ver.major1, s_c6_ver.minor1, s_c6_ver.patch1);
    }

    wifi_config_t ap = {
        .ap = {
            .ssid = WIFI_OTA_AP_SSID,
            .ssid_len = strlen(WIFI_OTA_AP_SSID),
            .password = WIFI_OTA_AP_PASSWORD,
            .channel = WIFI_OTA_AP_CHANNEL,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .max_connection = WIFI_OTA_AP_MAX_STATIONS,
            .pmf_cfg = { .required = false },
        },
    };
    if (esp_wifi_set_mode(WIFI_MODE_AP) != ESP_OK ||
        esp_wifi_set_config(WIFI_IF_AP, &ap) != ESP_OK ||
        esp_wifi_start() != ESP_OK) {
        ESP_LOGE(TAG, "couldn't start the access point");
        return false;
    }
    return true;
}

// ---------------------------------------------------------------- the window

static void window_task(void *arg)
{
    int64_t start = esp_timer_get_time();

    if (!start_wifi() || !start_http()) {
        if (s_pending) {
            roll_back_now("the new firmware can't bring up Wi-Fi");
        }
        ESP_LOGW(TAG, "no update window this boot");
        if (s_safe_mode) {
            vTaskDelay(pdMS_TO_TICKS(WIFI_OTA_MAX_S * 1000LL));
            esp_restart();
        }
        vTaskDelete(NULL);
    }
    note_activity();
    ESP_LOGI(TAG, "update window open: join \"%s\", then http://192.168.4.1/%s",
             WIFI_OTA_AP_SSID, s_pending ? " to confirm the new firmware" : "");

    bool transport_ok = true;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        int64_t now = esp_timer_get_time();

        if (s_transport_failed) {
            transport_ok = false;
            if (s_pending) {
                roll_back_now("lost the C6 link before the new firmware was confirmed");
            }
            ESP_LOGE(TAG, "lost the link to the C6; closing the update window");
            break;
        }
        if (s_pending || s_updating) {
            continue;       // ended by the rollback timer, or by the reboot after the upload
        }
        if (s_stations > 0) {
            s_last_activity_us = now;
        }
        bool idle = (now - start > WIFI_OTA_WINDOW_S * 1000000LL) &&
                    (now - s_last_activity_us > WIFI_OTA_IDLE_S * 1000000LL);
        bool capped = now - start > WIFI_OTA_MAX_S * 1000000LL;
        if (capped || (idle && !s_safe_mode)) {
            break;
        }
    }

    httpd_stop(s_httpd);
    if (transport_ok) {
        esp_wifi_stop();
        esp_wifi_deinit();
    }
    if (s_safe_mode) {
        ESP_LOGW(TAG, "safe mode timed out; retrying a normal boot");
        esp_restart();
    }
    ESP_LOGI(TAG, "update window closed, Wi-Fi off");
    vTaskDelete(NULL);
}

void wifi_ota_start(void)
{
    const esp_app_desc_t *app = esp_app_get_description();
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    s_pending = esp_ota_get_state_partition(running, &state) == ESP_OK &&
                state == ESP_OTA_IMG_PENDING_VERIFY;
    esp_reset_reason_t why = esp_reset_reason();
    ESP_LOGI(TAG, "firmware %s in %s%s, last reset: %s", app->version, running->label,
             s_pending ? " (new, unconfirmed)" : "", reset_reason_str(why));

    // Boot-loop detection. RTC_NOINIT memory holds garbage after power loss, hence the magic.
    bool crashed = why == ESP_RST_PANIC || why == ESP_RST_INT_WDT ||
                   why == ESP_RST_TASK_WDT || why == ESP_RST_WDT;
    if (s_crash_magic != WIFI_OTA_CRASH_MAGIC || !crashed) {
        s_crash_magic = WIFI_OTA_CRASH_MAGIC;
        s_crash_count = 0;
    }
    if (crashed) {
        s_crash_count++;
    }
    s_safe_mode = !s_pending && s_crash_count >= WIFI_OTA_SAFE_MODE_CRASHES;
    const esp_timer_create_args_t stable_args = { .callback = stable_cb, .name = "ota_stable" };
    esp_timer_handle_t stable_timer;
    if (!s_safe_mode && esp_timer_create(&stable_args, &stable_timer) == ESP_OK) {
        esp_timer_start_once(stable_timer, WIFI_OTA_STABLE_S * 1000000LL);
    }

    if (why == ESP_RST_DEEPSLEEP && !s_pending) {
        return;     // a crow visit: Wi-Fi stays off
    }

    const esp_timer_create_args_t reboot_args = { .callback = reboot_cb, .name = "ota_reboot" };
    ESP_ERROR_CHECK(esp_timer_create(&reboot_args, &s_reboot_timer));
    if (s_pending) {
        const esp_timer_create_args_t rb_args = { .callback = rollback_timeout_cb, .name = "ota_rollback" };
        ESP_ERROR_CHECK(esp_timer_create(&rb_args, &s_rollback_timer));
        ESP_ERROR_CHECK(esp_timer_start_once(s_rollback_timer, WIFI_OTA_CONFIRM_TIMEOUT_S * 1000000LL));
    }

    if (xTaskCreate(window_task, "wifi_ota", WIFI_OTA_TASK_STACK, NULL,
                    WIFI_OTA_TASK_PRIORITY, NULL) != pdPASS) {
        ESP_LOGE(TAG, "couldn't create the update task");
        if (s_pending) {
            roll_back_now("no update task");
        }
        return;
    }

    if (s_safe_mode) {
        ESP_LOGE(TAG, "SAFE MODE: %" PRIu32 " crash reboots in a row; vision stays off, "
                 "update window open for up to %d min", s_crash_count, WIFI_OTA_MAX_S / 60);
        while (true) {
            vTaskDelay(portMAX_DELAY);      // keep app_main from reaching the crash
        }
    }
}
