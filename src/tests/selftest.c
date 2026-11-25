// Power-on self-test for the main junction box. See include/selftest.h.
//
// Flash once, then power-cycle (or press EN) to re-run. Results go to:
//   - serial (115200), one line per check plus a summary
//   - Notehub, as a note in "selftest.qo" (synced immediately)
//   - the signal LEDs: green = all checks passed, red = at least one failed
//
// What can and can't be verified automatically:
//   MEASURED: I2C devices answer, MCP23017 registers and output latches read back
//             correctly, both HX711s produce sane data, Notecard answers and reports
//             its signal and supply voltage, vision board answers a PING, NVS works.
//   EXERCISED ONLY: LEDs, COB strip, buzzer, solenoid, servos, gate. Nothing senses
//             these, so the test drives them and you confirm by eye and ear.
//
// Self-contained on purpose: it sets up the PIR and HX711 pins itself, because
// pir_init()/weight_init() start interrupts/tasks that post to the event bus, and
// it talks to the Notecard with its own minimal I2C helper because notecard.c
// isn't written yet.

#include "selftest.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_chip_info.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"

#include "pinout.h"
#include "i2c_bus.h"
#include "mcp23017.h"
#include "weight.h"
#include "servo.h"
#include "buzzer.h"
#include "config.h"
#include "vision_link.h"

#define TAG "selftest"

// ---------------------------------------------------------------------------
// Options
// ---------------------------------------------------------------------------
#define SELFTEST_EXERCISE_OUTPUTS  1    // flash LEDs + COB, beep, click solenoid, wiggle servos
#define SELFTEST_EXERCISE_GATE     1    // short gate extend then retract (both actuators together)
#define SELFTEST_SERVO_WIGGLE_DEG  20   // small move from 0 deg and back; hoppers are not dosed
#define SELFTEST_PRODUCT_UID       "com.gmail.dccrooks78:crowbox"   // Notehub ProductUID. "" = keep whatever the Notecard has
                                        // (set once in the Blues Playground with hub.set)
#define SELFTEST_SYNC_WAIT_S       90   // how long to watch the Notehub sync before giving up
#define SELFTEST_HX711_SAMPLES     20

// MCP23017 registers (IOCON.BANK = 0)
#define ST_MCP_IODIRA  0x00
#define ST_MCP_IODIRB  0x01
#define ST_MCP_OLATA   0x14
#define ST_MCP_OLATB   0x15

// ---------------------------------------------------------------------------
// Result bookkeeping
// ---------------------------------------------------------------------------
typedef enum { R_PASS, R_WARN, R_FAIL } res_t;

static int  s_pass, s_warn, s_fail;
static char s_failed[128];   // comma-separated names of failed checks, sent to Notehub
static char s_warned[128];

static void list_append(char *list, size_t cap, const char *name) {
    size_t len = strlen(list);
    if (len + strlen(name) + 2 >= cap) return;
    if (len) strcat(list, ",");
    strcat(list, name);
}

static void check(const char *name, res_t r, const char *fmt, ...) {
    char detail[192];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(detail, sizeof detail, fmt, ap);
    va_end(ap);
    switch (r) {
        case R_PASS: s_pass++; ESP_LOGI(TAG, "[PASS] %-14s %s", name, detail); break;
        case R_WARN: s_warn++; list_append(s_warned, sizeof s_warned, name);
                     ESP_LOGW(TAG, "[WARN] %-14s %s", name, detail); break;
        case R_FAIL: s_fail++; list_append(s_failed, sizeof s_failed, name);
                     ESP_LOGE(TAG, "[FAIL] %-14s %s", name, detail); break;
    }
}

static void delay_ms(uint32_t ms) {
    TickType_t t = pdMS_TO_TICKS(ms);
    vTaskDelay(t ? t : 1);
}

static int64_t now_ms(void) { return esp_timer_get_time() / 1000; }

// ---------------------------------------------------------------------------
// Values gathered for the Notehub note
// ---------------------------------------------------------------------------
typedef struct {
    bool    ok;
    int     samples, timeouts;
    int32_t min, max;
    double  mean, sd;
} hx_stats_t;

static struct {
    uint32_t   run;
    bool       i2c_mcp, i2c_nc, mcp_regs, mcp_latch;
    hx_stats_t hx_dep, hx_rew;
    int        pir;
    int        ir;
    bool       vision;
    bool       nvs;
    int        stage;
    uint32_t   deposits;
    bool       nc_ok;
    double     volt, temp;
    int        bars, rssi;
    char       sku[24];
} R = { .pir = -1, .ir = -1, .volt = -1, .temp = -99, .bars = -1, .rssi = 0 };

static i2c_master_dev_handle_t s_mcp_dev;
static i2c_master_dev_handle_t s_nc_dev;

// ---------------------------------------------------------------------------
// 1. System + NVS
// ---------------------------------------------------------------------------
static const char *reset_reason_str(esp_reset_reason_t r) {
    switch (r) {
        case ESP_RST_POWERON:   return "power-on";
        case ESP_RST_EXT:       return "external pin";
        case ESP_RST_SW:        return "software";
        case ESP_RST_PANIC:     return "panic/crash";
        case ESP_RST_INT_WDT:   return "interrupt watchdog";
        case ESP_RST_TASK_WDT:  return "task watchdog";
        case ESP_RST_WDT:       return "other watchdog";
        case ESP_RST_DEEPSLEEP: return "deep sleep wake";
        case ESP_RST_BROWNOUT:  return "BROWNOUT";
        default:                return "other";
    }
}

static void test_system_and_nvs(void) {
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    esp_reset_reason_t rr = esp_reset_reason();
    ESP_LOGI(TAG, "Build %s %s | chip rev %d, %d cores | free heap %lu | reset: %s",
             __DATE__, __TIME__, chip.revision, chip.cores,
             (unsigned long)esp_get_free_heap_size(), reset_reason_str(rr));

    // A brownout or crash reset means the last run died, not that this one failed.
    if (rr == ESP_RST_BROWNOUT) {
        check("reset", R_WARN, "last reset was a BROWNOUT: the 5V rail sagged (servo stall? weak supply?)");
    } else if (rr == ESP_RST_PANIC || rr == ESP_RST_TASK_WDT || rr == ESP_RST_INT_WDT) {
        check("reset", R_WARN, "last reset was a crash/watchdog (%s)", reset_reason_str(rr));
    } else {
        check("reset", R_PASS, "%s", reset_reason_str(rr));
    }

    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        check("nvs", R_FAIL, "nvs_flash_init: %s (not erasing it: that would wipe training progress)",
              esp_err_to_name(err));
        return;
    }
    nvs_handle_t h;
    if (nvs_open("selftest", NVS_READWRITE, &h) == ESP_OK) {
        nvs_get_u32(h, "runs", &R.run);
        R.run++;
        nvs_set_u32(h, "runs", R.run);
        nvs_commit(h);
        nvs_close(h);
    }
    config_init();
    R.nvs = true;
    R.stage = (int)g_config.stage;
    R.deposits = g_config.successful_deposit_count;
    check("nvs", R_PASS, "self-test run #%lu | training stage %d, %lu successful deposits",
          (unsigned long)R.run, R.stage, (unsigned long)R.deposits);
}

// ---------------------------------------------------------------------------
// 2. I2C bus
// ---------------------------------------------------------------------------
static void test_i2c(void) {
    if (i2c_bus_get_handle() == NULL) i2c_bus_init();
    i2c_master_bus_handle_t bus = i2c_bus_get_handle();
    if (bus == NULL) {
        check("i2c_bus", R_FAIL, "could not create the I2C bus on GPIO%d/%d", PIN_I2C_SDA, PIN_I2C_SCL);
        return;
    }

    char others[64] = "";
    for (uint16_t a = 0x08; a <= 0x77; a++) {
        if (i2c_master_probe(bus, a, 50) != ESP_OK) continue;
        if (a == I2C_ADDR_MCP23017)      R.i2c_mcp = true;
        else if (a == I2C_ADDR_NOTECARD) R.i2c_nc = true;
        else {
            char s[8];
            snprintf(s, sizeof s, "0x%02X", a);
            list_append(others, sizeof others, s);
        }
    }
    // The Notecard takes a few seconds to boot when it powers up with the ESP32,
    // and won't answer until then. Give it up to 10 s.
    if (!R.i2c_nc) {
        ESP_LOGI(TAG, "Notecard not answering yet; waiting up to 10 s for it to boot...");
        for (int i = 0; i < 20 && !R.i2c_nc; i++) {
            delay_ms(500);
            R.i2c_nc = i2c_master_probe(bus, I2C_ADDR_NOTECARD, 50) == ESP_OK;
        }
    }
    check("i2c_mcp23017", R.i2c_mcp ? R_PASS : R_FAIL,
          R.i2c_mcp ? "answers at 0x20" : "no answer at 0x20: check power, SDA/SCL (GPIO21/22), A0-A2 to GND");
    check("i2c_notecard", R.i2c_nc ? R_PASS : R_FAIL,
          R.i2c_nc ? "answers at 0x17" : "no answer at 0x17: check Notecarrier V+ power, Notecard seated, SDA/SCL");
    if (others[0]) check("i2c_extra", R_WARN, "unexpected device(s) on the bus: %s", others);

    i2c_device_config_t dc = { .dev_addr_length = I2C_ADDR_BIT_LEN_7, .scl_speed_hz = 100000 };
    if (R.i2c_mcp) {
        dc.device_address = I2C_ADDR_MCP23017;
        i2c_master_bus_add_device(bus, &dc, &s_mcp_dev);
    }
    if (R.i2c_nc) {
        dc.device_address = I2C_ADDR_NOTECARD;
        i2c_master_bus_add_device(bus, &dc, &s_nc_dev);
    }
}

// ---------------------------------------------------------------------------
// 3. MCP23017: registers + output latches
// ---------------------------------------------------------------------------
static bool mcp_read_reg(uint8_t reg, uint8_t *val) {
    return i2c_master_transmit_receive(s_mcp_dev, &reg, 1, val, 1, 100) == ESP_OK;
}

// Sets one output through the normal driver, then reads the chip's output latch back.
static bool mcp_set_and_verify(bool port, uint8_t bit, bool high) {
    mcp23017_set_pin(port, bit, high);
    uint8_t olat;
    if (!mcp_read_reg(port ? ST_MCP_OLATB : ST_MCP_OLATA, &olat)) return false;
    bool ok = (((olat >> bit) & 1) == (high ? 1 : 0));
    if (!ok) R.mcp_latch = false;
    return ok;
}

static void test_mcp(void) {
    if (!R.i2c_mcp) return;
    mcp23017_init();
    uint8_t ia = 0, ib = 0;
    bool rd = mcp_read_reg(ST_MCP_IODIRA, &ia) && mcp_read_reg(ST_MCP_IODIRB, &ib);
    R.mcp_regs = rd && ia == 0x0F && ib == 0xE0;
    check("mcp_regs", R.mcp_regs ? R_PASS : R_FAIL,
          "IODIRA=0x%02X (want 0x0F) IODIRB=0x%02X (want 0xE0)%s", ia, ib, rd ? "" : " [read failed]");

    // Everything off, and confirm every output latch reads back low.
    R.mcp_latch = true;
    const uint8_t a_out[] = { MCP_BIT_LED_GREEN, MCP_BIT_LED_RED, MCP_BIT_LED_BLUE, MCP_BIT_COB_LED };
    const uint8_t b_out[] = { MCP_BIT_ACT_A_IN1, MCP_BIT_ACT_A_IN2, MCP_BIT_ACT_B_IN1,
                              MCP_BIT_ACT_B_IN2, MCP_BIT_SOLENOID };
    for (size_t i = 0; i < sizeof a_out; i++) mcp_set_and_verify(0, a_out[i], false);
    for (size_t i = 0; i < sizeof b_out; i++) mcp_set_and_verify(1, b_out[i], false);
    check("mcp_outputs_off", R.mcp_latch ? R_PASS : R_FAIL,
          R.mcp_latch ? "all 9 outputs latched low" : "an output latch did not read back low");

    // IR beams on GPA0-3 (hardware paused, so informational only).
    R.ir = mcp23017_read_port(0) & 0x0F;
    ESP_LOGI(TAG, "[INFO] ir_beams       GPA3..0 = %d%d%d%d (1 = beam unbroken; sensors not installed yet)",
             (R.ir >> 3) & 1, (R.ir >> 2) & 1, (R.ir >> 1) & 1, R.ir & 1);
}

// ---------------------------------------------------------------------------
// 4. HX711 x2
// ---------------------------------------------------------------------------
static void hx_pins(gpio_num_t sck, gpio_num_t dout) {
    gpio_config_t s = { .pin_bit_mask = 1ULL << sck,  .mode = GPIO_MODE_OUTPUT };
    gpio_config_t d = { .pin_bit_mask = 1ULL << dout, .mode = GPIO_MODE_INPUT,
                        .pull_up_en = GPIO_PULLUP_ENABLE };
    gpio_config(&s);
    gpio_config(&d);
    gpio_set_level(sck, 0);   // SCK held high >60 us powers the HX711 down
}

static bool hx_wait_ready(gpio_num_t dout, int timeout_ms) {
    int64_t t0 = now_ms();
    while (gpio_get_level(dout) != 0) {
        if (now_ms() - t0 > timeout_ms) return false;
        delay_ms(5);
    }
    return true;
}

static hx_stats_t hx_measure(weight_sensor_t sensor, gpio_num_t sck, gpio_num_t dout) {
    hx_stats_t st = { .min = INT32_MAX, .max = INT32_MIN };
    hx_pins(sck, dout);
    delay_ms(500);                                        // HX711 settles after power-up
    if (hx_wait_ready(dout, 400)) weight_read_raw(sensor); // throw away the first conversion

    double sum = 0, sumsq = 0;
    bool railed = false;
    for (int i = 0; i < SELFTEST_HX711_SAMPLES; i++) {
        if (!hx_wait_ready(dout, 400)) { st.timeouts++; continue; }   // 10 SPS -> ~100 ms each
        int32_t v = weight_read_raw(sensor);
        if (v == 0x7FFFFF || v == -0x800000) railed = true;
        st.samples++;
        sum += v;
        sumsq += (double)v * v;
        if (v < st.min) st.min = v;
        if (v > st.max) st.max = v;
    }
    if (st.samples) {
        st.mean = sum / st.samples;
        double var = sumsq / st.samples - st.mean * st.mean;
        st.sd = var > 0 ? sqrt(var) : 0;
    }
    st.ok = st.timeouts == 0 && st.samples > 1 && st.max != st.min && !railed;
    if (railed) st.ok = false;
    return st;
}

static void report_hx(const char *name, const hx_stats_t *st) {
    if (st->samples == 0) {
        check(name, R_FAIL, "no data: DOUT never went low. Check HX711 power (3.3V), SCK/DOUT wires");
    } else if (!st->ok) {
        check(name, R_FAIL, "%d samples, %d timeouts, raw %ld..%ld (stuck or at the rail)",
              st->samples, st->timeouts, (long)st->min, (long)st->max);
    } else if (st->sd > 2000) {
        check(name, R_WARN, "mean %.0f sd %.0f: very noisy (loose wire? motor noise? something touching?)",
              st->mean, st->sd);
    } else {
        // A raw reading can't show whether a load cell is attached: floating inputs also
        // give steady-looking numbers. Only putting weight on the cell proves that.
        check(name, R_PASS, "HX711 chip responding: mean %.0f sd %.0f raw counts, range %ld..%ld "
              "(load cell presence not checked: press on it and the mean should move)",
              st->mean, st->sd, (long)st->min, (long)st->max);
    }
}

static void test_hx711(void) {
    R.hx_dep = hx_measure(WEIGHT_SENSOR_DEPOSIT, PIN_HX711_1_SCK, PIN_HX711_1_DOUT);
    report_hx("hx711_deposit", &R.hx_dep);
    R.hx_rew = hx_measure(WEIGHT_SENSOR_REWARD, PIN_HX711_2_SCK, PIN_HX711_2_DOUT);
    report_hx("hx711_reward", &R.hx_rew);
}

// ---------------------------------------------------------------------------
// 5. PIR (level only; no ISR, so nothing posts to the event bus)
// ---------------------------------------------------------------------------
static void test_pir(void) {
    gpio_config_t p = { .pin_bit_mask = 1ULL << PIN_PIR, .mode = GPIO_MODE_INPUT };
    gpio_config(&p);
    int highs = 0;
    for (int i = 0; i < 20; i++) {            // watch for 2 s
        highs += gpio_get_level(PIN_PIR);
        delay_ms(100);
    }
    R.pir = gpio_get_level(PIN_PIR);
    ESP_LOGI(TAG, "[INFO] pir            level %d, high in %d/20 samples over 2 s "
             "(AM312 needs ~30 s after power-up to settle; wave a hand to see it go high)", R.pir, highs);
}

// ---------------------------------------------------------------------------
// 6. Vision board (UART PING)
// ---------------------------------------------------------------------------
static void test_vision(void) {
    vision_link_init();                       // also sends a PING that wakes a sleeping P4
    delay_ms(3000);                           // P4 boots and announces READY in ~1-2 s
    UART_msg_t reply;
    for (int i = 0; i < 3 && !R.vision; i++) {
        if (vision_link_request(MSG_PING, &reply, 1500) && reply.type == MSG_OK) R.vision = true;
        else delay_ms(1000);
    }
    check("vision_link", R.vision ? R_PASS : R_FAIL,
          R.vision ? "P4 answered PING" :
          "no PING reply: check the P4 is powered (vision-box buck), TX/RX on GPIO17/16 crossed, common GND");
}

// ---------------------------------------------------------------------------
// 7. Outputs that can only be watched/heard
// ---------------------------------------------------------------------------
static void test_outputs(void) {
#if SELFTEST_EXERCISE_OUTPUTS
    ESP_LOGI(TAG, "---- Exercising outputs: watch and listen ----");
    bool latch_ok = true;
    if (R.i2c_mcp) {
        const struct { uint8_t bit; const char *name; } leds[] = {
            { MCP_BIT_LED_GREEN, "green LED" }, { MCP_BIT_LED_RED, "red LED" },
            { MCP_BIT_LED_BLUE, "blue LED" },   { MCP_BIT_COB_LED, "COB strip" },
        };
        for (size_t i = 0; i < sizeof leds / sizeof leds[0]; i++) {
            ESP_LOGI(TAG, "  %s ON", leds[i].name);
            latch_ok &= mcp_set_and_verify(0, leds[i].bit, true);
            delay_ms(leds[i].bit == MCP_BIT_COB_LED ? 1500 : 600);
            latch_ok &= mcp_set_and_verify(0, leds[i].bit, false);
        }
    }

    ESP_LOGI(TAG, "  buzzer: accept tone");
    buzzer_init();
    buzzer_play_accept();

    servo_init();                              // all servos go to 0 deg
    delay_ms(800);
    const char *servo_names[] = { "hopper 1 (GPIO13)", "hopper 2 (GPIO19)", "hopper 3 (GPIO18)",
                                  "hopper 4 (GPIO14, not installed)" };
    for (uint8_t ch = LEDC_CHAN_SERVO_HOPPER1; ch <= LEDC_CHAN_SERVO_HOPPER4; ch++) {
        ESP_LOGI(TAG, "  servo %s: 0 -> %d -> 0 deg", servo_names[ch], SELFTEST_SERVO_WIGGLE_DEG);
        servo_set_angle(ch, SELFTEST_SERVO_WIGGLE_DEG);
        delay_ms(600);
        servo_set_angle(ch, 0);
        delay_ms(600);                         // one servo at a time (stall current)
    }

    if (R.i2c_mcp) {
        // Same order as the real trapdoor sequence: unlock, lift, lower, lock.
        ESP_LOGI(TAG, "  solenoid UNLOCK (listen for the click), trapdoor servo 0 -> %d -> 0 deg, LOCK",
                 SELFTEST_SERVO_WIGGLE_DEG);
        latch_ok &= mcp_set_and_verify(1, MCP_BIT_SOLENOID, true);
        delay_ms(150);
        servo_trapdoor_set_angle(SELFTEST_SERVO_WIGGLE_DEG);
        delay_ms(600);
        servo_trapdoor_set_angle(0);
        delay_ms(600);
        latch_ok &= mcp_set_and_verify(1, MCP_BIT_SOLENOID, false);
    }

#if SELFTEST_EXERCISE_GATE
    if (R.i2c_mcp) {
        // Both actuators together (they lift one sleeve). Short extend, longer retract,
        // so it ends closed. If the sleeve twists, one actuator is wired backwards.
        ESP_LOGI(TAG, "  gate: both actuators EXTEND 1.5 s, then RETRACT 3 s");
        latch_ok &= mcp_set_and_verify(1, MCP_BIT_ACT_A_IN2, false);
        latch_ok &= mcp_set_and_verify(1, MCP_BIT_ACT_B_IN2, false);
        latch_ok &= mcp_set_and_verify(1, MCP_BIT_ACT_A_IN1, true);
        latch_ok &= mcp_set_and_verify(1, MCP_BIT_ACT_B_IN1, true);
        delay_ms(1500);
        latch_ok &= mcp_set_and_verify(1, MCP_BIT_ACT_A_IN1, false);
        latch_ok &= mcp_set_and_verify(1, MCP_BIT_ACT_B_IN1, false);
        delay_ms(500);
        latch_ok &= mcp_set_and_verify(1, MCP_BIT_ACT_A_IN2, true);
        latch_ok &= mcp_set_and_verify(1, MCP_BIT_ACT_B_IN2, true);
        delay_ms(3000);
        latch_ok &= mcp_set_and_verify(1, MCP_BIT_ACT_A_IN2, false);
        latch_ok &= mcp_set_and_verify(1, MCP_BIT_ACT_B_IN2, false);
    }
#endif

    if (R.i2c_mcp) {
        check("mcp_switching", latch_ok ? R_PASS : R_FAIL,
              latch_ok ? "every output latch followed its on/off command (loads themselves: confirm by eye/ear)"
                       : "an MCP23017 output latch did not follow its command");
    }
    ESP_LOGI(TAG, "---- Outputs done ----");
#else
    ESP_LOGI(TAG, "[INFO] outputs        not exercised (SELFTEST_EXERCISE_OUTPUTS = 0)");
#endif
}

// ---------------------------------------------------------------------------
// 8. Notecard: minimal serial-over-I2C JSON helper (test-only; the real driver
//    should use Blues' note-c library). Protocol, from note-c:
//    send:    write [len][len bytes], len <= 30, pause 250 ms after each 250 bytes
//    receive: write [0][n], then read n+2 bytes = [bytes still waiting][n][n data bytes]
//             (n = 0 just asks how many bytes are waiting)
// ---------------------------------------------------------------------------
#define NC_CHUNK 30

static bool nc_tx(const char *data, size_t len) {
    uint8_t buf[NC_CHUNK + 1];
    size_t sent = 0, seg = 0;
    while (sent < len) {
        size_t n = len - sent > NC_CHUNK ? NC_CHUNK : len - sent;
        buf[0] = (uint8_t)n;
        memcpy(buf + 1, data + sent, n);
        esp_err_t e = ESP_FAIL;
        for (int t = 0; t < 5 && e != ESP_OK; t++) {
            e = i2c_master_transmit(s_nc_dev, buf, n + 1, 100);
            if (e != ESP_OK) delay_ms(20);
        }
        if (e != ESP_OK) return false;
        sent += n;
        seg += n;
        if (seg >= 250) { delay_ms(250); seg = 0; }
        else delay_ms(5);
    }
    return true;
}

// Reads `want` bytes (0 = just poll). Returns false on bus error.
static bool nc_rx(uint8_t *out, uint8_t want, uint8_t *waiting) {
    uint8_t q[2] = { 0, want };
    if (i2c_master_transmit(s_nc_dev, q, 2, 100) != ESP_OK) return false;
    delay_ms(2);
    uint8_t buf[NC_CHUNK + 2];
    if (i2c_master_receive(s_nc_dev, buf, want + 2, 100) != ESP_OK) return false;
    *waiting = buf[0];
    if (buf[1] != want) return false;
    if (want) memcpy(out, buf + 2, want);
    return true;
}

static void nc_drain(void) {
    uint8_t waiting = 0, junk[NC_CHUNK];
    for (int i = 0; i < 50; i++) {
        if (!nc_rx(NULL, 0, &waiting) || waiting == 0) return;
        nc_rx(junk, waiting > NC_CHUNK ? NC_CHUNK : waiting, &waiting);
    }
}

// Sends one JSON request (must end in '\n') and reads the one-line JSON response.
static bool nc_request(const char *req, char *resp, size_t cap, int timeout_ms) {
    resp[0] = 0;
    if (!s_nc_dev) return false;
    nc_drain();
    if (!nc_tx(req, strlen(req))) return false;
    size_t got = 0;
    uint8_t waiting = 0;
    int64_t t0 = now_ms();
    while (now_ms() - t0 < timeout_ms) {
        if (waiting == 0) {
            if (!nc_rx(NULL, 0, &waiting) || waiting == 0) { delay_ms(50); continue; }
        }
        uint8_t chunk[NC_CHUNK];
        uint8_t n = waiting > NC_CHUNK ? NC_CHUNK : waiting;
        if (!nc_rx(chunk, n, &waiting)) { waiting = 0; delay_ms(20); continue; }
        for (uint8_t i = 0; i < n; i++) {
            if (got < cap - 1) resp[got++] = (char)chunk[i];
            if (chunk[i] == '\n') { resp[got] = 0; return true; }
        }
    }
    resp[got] = 0;
    return false;
}

static bool js_str(const char *js, const char *key, char *out, size_t cap) {
    char pat[40];
    snprintf(pat, sizeof pat, "\"%s\":\"", key);
    const char *p = strstr(js, pat);
    if (!p) return false;
    p += strlen(pat);
    size_t i = 0;
    while (*p && *p != '"' && i < cap - 1) out[i++] = *p++;
    out[i] = 0;
    return true;
}

static bool js_num(const char *js, const char *key, double *v) {
    char pat[40];
    snprintf(pat, sizeof pat, "\"%s\":", key);
    const char *p = strstr(js, pat);
    if (!p) return false;
    p += strlen(pat);
    char *end;
    double d = strtod(p, &end);
    if (end == p) return false;
    *v = d;
    return true;
}

static void test_notecard(void) {
    static char resp[768];
    if (!R.i2c_nc) {
        check("notecard", R_FAIL, "not on the I2C bus, so no report can be sent to Notehub");
        return;
    }

    if (!nc_request("{\"req\":\"card.version\"}\n", resp, sizeof resp, 5000) || strstr(resp, "\"err\"")) {
        check("notecard", R_FAIL, "card.version got no valid reply (%.80s)", resp);
        return;
    }
    char ver[40] = "?", dev[32] = "?";
    js_str(resp, "version", ver, sizeof ver);
    js_str(resp, "device", dev, sizeof dev);
    js_str(resp, "sku", R.sku, sizeof R.sku);
    R.nc_ok = true;
    check("notecard", R_PASS, "%s | %s | %s", R.sku, dev, ver);

    // Which Notehub project it reports to.
    char product[64] = "";
    if (nc_request("{\"req\":\"hub.get\"}\n", resp, sizeof resp, 5000)) js_str(resp, "product", product, sizeof product);
    if (SELFTEST_PRODUCT_UID[0] && strcmp(product, SELFTEST_PRODUCT_UID) != 0) {
        nc_request("{\"req\":\"hub.set\",\"product\":\"" SELFTEST_PRODUCT_UID "\",\"mode\":\"periodic\"}\n",
                   resp, sizeof resp, 5000);
        snprintf(product, sizeof product, "%s", SELFTEST_PRODUCT_UID);
    }
    check("notehub_project", product[0] ? R_PASS : R_FAIL,
          product[0] ? "%s" : "no ProductUID set: run hub.set once in the Blues Playground, "
                              "or fill SELFTEST_PRODUCT_UID in selftest.c", product);

    // Supply voltage as the Notecard sees it (its V+ = the 5V rail).
    if (nc_request("{\"req\":\"card.voltage\"}\n", resp, sizeof resp, 5000)) js_num(resp, "value", &R.volt);
    if (R.volt < 0)          check("supply_5v", R_WARN, "card.voltage gave no reading");
    else if (R.volt < 4.6)   check("supply_5v", R_FAIL, "%.2f V at the Notecarrier: rail too low (buck set low? sagging?)", R.volt);
    else if (R.volt > 5.4)   check("supply_5v", R_FAIL, "%.2f V at the Notecarrier: rail too high (max 5.5 V)", R.volt);
    else                     check("supply_5v", R_PASS, "%.2f V at the Notecarrier", R.volt);

    if (nc_request("{\"req\":\"card.temp\"}\n", resp, sizeof resp, 5000)) js_num(resp, "value", &R.temp);
    ESP_LOGI(TAG, "[INFO] temperature    %.1f C inside the junction box (Notecard sensor)", R.temp);

    // Cellular signal. Right after power-up the modem may not have registered yet.
    double bars = -1, rssi = 0;
    for (int i = 0; i < 6; i++) {
        if (nc_request("{\"req\":\"card.wireless\"}\n", resp, sizeof resp, 5000) && js_num(resp, "bars", &bars)) break;
        delay_ms(5000);
    }
    js_num(resp, "rssi", &rssi);
    R.bars = (int)bars;
    R.rssi = (int)rssi;
    if (R.bars < 0)       check("cell_signal", R_WARN, "modem not registered yet (normal in the first minutes after power-up)");
    else if (R.bars == 0) check("cell_signal", R_WARN, "0 bars, rssi %d dBm: try the flexible antenna", R.rssi);
    else                  check("cell_signal", R_PASS, "%d bars, rssi %d dBm", R.bars, R.rssi);
}

// ---------------------------------------------------------------------------
// 9. Send the report to Notehub and watch the sync
// ---------------------------------------------------------------------------
static const char *tf(bool b) { return b ? "true" : "false"; }

static void send_report(void) {
    if (!R.nc_ok) return;
    static char req[1024], resp[512];
    snprintf(req, sizeof req,
        "{\"req\":\"note.add\",\"file\":\"selftest.qo\",\"sync\":true,\"body\":{"
        "\"run\":%lu,\"reset\":\"%s\",\"build\":\"%s %s\","
        "\"pass\":%d,\"warn\":%d,\"fail\":%d,\"failed\":\"%s\",\"warned\":\"%s\","
        "\"i2c_mcp\":%s,\"i2c_nc\":%s,\"mcp_regs\":%s,\"mcp_latch\":%s,"
        "\"hx_dep_ok\":%s,\"hx_dep_mean\":%.0f,\"hx_dep_sd\":%.0f,\"hx_dep_timeouts\":%d,"
        "\"hx_rew_ok\":%s,\"hx_rew_mean\":%.0f,\"hx_rew_sd\":%.0f,\"hx_rew_timeouts\":%d,"
        "\"pir\":%d,\"ir\":%d,\"vision\":%s,\"nvs\":%s,\"stage\":%d,\"deposits\":%lu,"
        "\"volt\":%.2f,\"temp\":%.1f,\"bars\":%d,\"rssi\":%d,\"heap\":%lu}}\n",
        (unsigned long)R.run, reset_reason_str(esp_reset_reason()), __DATE__, __TIME__,
        s_pass, s_warn, s_fail, s_failed, s_warned,
        tf(R.i2c_mcp), tf(R.i2c_nc), tf(R.mcp_regs), tf(R.mcp_latch),
        tf(R.hx_dep.ok), R.hx_dep.mean, R.hx_dep.sd, R.hx_dep.timeouts,
        tf(R.hx_rew.ok), R.hx_rew.mean, R.hx_rew.sd, R.hx_rew.timeouts,
        R.pir, R.ir, tf(R.vision), tf(R.nvs), R.stage, (unsigned long)R.deposits,
        R.volt, R.temp, R.bars, R.rssi, (unsigned long)esp_get_free_heap_size());

    if (!nc_request(req, resp, sizeof resp, 10000) || strstr(resp, "\"err\"")) {
        ESP_LOGE(TAG, "note.add failed: %.120s", resp);
        return;
    }
    ESP_LOGI(TAG, "Report queued on the Notecard (selftest.qo). Watching the sync for up to %d s...",
             SELFTEST_SYNC_WAIT_S);

    char status[96] = "";
    int64_t t0 = now_ms();
    bool done = false;
    while (!done && now_ms() - t0 < SELFTEST_SYNC_WAIT_S * 1000) {
        delay_ms(5000);
        if (!nc_request("{\"req\":\"hub.sync.status\"}\n", resp, sizeof resp, 5000)) continue;
        js_str(resp, "status", status, sizeof status);
        ESP_LOGI(TAG, "  sync: %s", status);
        if (strstr(resp, "\"err\"")) break;
        done = strstr(status, "completed") || strstr(status, "sync-end");
    }
    if (done) ESP_LOGI(TAG, "Report delivered to Notehub. Check Events in your project.");
    else ESP_LOGW(TAG, "Sync not confirmed within %d s. The note stays on the Notecard and "
                       "goes out on its next successful sync.", SELFTEST_SYNC_WAIT_S);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
void selftest_run(void) {
    ESP_LOGI(TAG, "================ CrowBox power-on self-test ================");
    s_pass = s_warn = s_fail = 0;
    s_failed[0] = s_warned[0] = 0;

    test_system_and_nvs();
    test_i2c();
    test_mcp();
    test_hx711();
    test_pir();
    test_vision();
    test_outputs();
    test_notecard();

    ESP_LOGI(TAG, "================ SUMMARY: %d pass, %d warn, %d fail ================",
             s_pass, s_warn, s_fail);
    if (s_fail) ESP_LOGE(TAG, "FAILED: %s", s_failed);
    if (s_warn) ESP_LOGW(TAG, "WARNINGS: %s", s_warned);

    // Leave a light on so the result is visible without a laptop.
    if (R.i2c_mcp) mcp23017_set_pin(0, s_fail ? MCP_BIT_LED_RED : MCP_BIT_LED_GREEN, true);

    send_report();
    ESP_LOGI(TAG, "Self-test done. Power-cycle or press EN to run it again.");
}
