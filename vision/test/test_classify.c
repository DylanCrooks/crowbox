// Laptop test for preprocess.c + classify_blob.c (no ESP-IDF, no camera).
// Synthetic 1280x960 RGB565 frames: a light pit floor with sensor noise, plus shapes.
// From vision/test/ in an MSYS2 UCRT64 terminal:
//   gcc -Wall -Wextra -I../main -I../../shared test_classify.c ../main/preprocess.c ../main/classify_blob.c -lm -o test_classify && ./test_classify
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "preprocess.h"
#include "classify.h"

#define W       1280
#define H       960
#define SCALE   4
#define GW      (W / SCALE)
#define GH      (H / SCALE)
#define MM_PX   (0.92f / SCALE)     // full-res mm per pixel, matching BLOB_MM_PER_PX

static uint8_t frame[W * H * 2];
static uint8_t gray[GW * GH];
static uint8_t ref[GW * GH];
static int failures;

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

static void put(int x, int y, uint8_t v)
{
    uint16_t p = rgb565(v, v, v);
    frame[2 * (y * W + x)] = p & 0xFF;          // little-endian, as preprocess assumes
    frame[2 * (y * W + x) + 1] = p >> 8;
}

static void floor_fill(int level)
{
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            int v = level + (rand() % 17) - 8;  // +-8 sensor noise
            put(x, y, (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v));
        }
}

// Shapes in mm, centered on the frame; value is the object's gray level.
static void disc(float diameter_mm, uint8_t v)
{
    float r = diameter_mm / 2 / MM_PX;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            float dx = x - W / 2.0f, dy = y - H / 2.0f;
            if (dx * dx + dy * dy <= r * r) put(x, y, v);
        }
}

static void square(float side_mm, uint8_t v)
{
    int half = (int)(side_mm / 2 / MM_PX);
    for (int y = H / 2 - half; y < H / 2 + half; y++)
        for (int x = W / 2 - half; x < W / 2 + half; x++) put(x, y, v);
}

static void ellipse(float a_mm, float b_mm, uint8_t v)
{
    float a = a_mm / 2 / MM_PX, b = b_mm / 2 / MM_PX;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            float dx = (x - W / 2.0f) / a, dy = (y - H / 2.0f) / b;
            if (dx * dx + dy * dy <= 1) put(x, y, v);
        }
}

static void make_reference(int level)
{
    floor_fill(level);
    preprocess_gray(frame, W, H, SCALE, ref);
}

static void check(const char *name, const uint8_t *reference, int want)
{
    classify_result_t r;
    preprocess_gray(frame, W, H, SCALE, gray);
    bool ok = classify_blob(gray, reference, &r);
    bool pass = ok && r.cls == want;
    printf("%-34s cls=%u conf=%2u  want=%d  %s\n", name, r.cls, r.confidence, want, pass ? "PASS" : "FAIL");
    if (!pass) failures++;
}

int main(void)
{
    srand(1);

    // preprocess: known colors -> gray
    put(0, 0, 255);
    preprocess_gray(frame, W, H, 1, gray);
    printf("white -> %u (want 255)\n", gray[0]);
    if (gray[0] != 255) failures++;

    if (!classify_blob_init(GW, GH)) {
        printf("init failed\n");
        return 1;
    }
    make_reference(200);

    floor_fill(200);                       check("empty pit, with reference", ref, CLASS_NONE);
    floor_fill(200); disc(24.26f, 90);     check("quarter (dark), with reference", ref, CLASS_COIN);
    floor_fill(200); disc(17.9f, 240);     check("dime (bright), with reference", ref, CLASS_COIN);
    floor_fill(220); disc(21.2f, 90);      check("nickel, exposure +20 vs reference", ref, CLASS_COIN);
    floor_fill(200); disc(24.26f, 90);     check("quarter, no reference", NULL, CLASS_COIN);
    floor_fill(200); square(21.5f, 90);    check("square, coin-sized", ref, CLASS_OTHER);
    floor_fill(200); ellipse(30, 15, 90);  check("2:1 ellipse", ref, CLASS_OTHER);
    floor_fill(200); disc(8, 90);          check("small disc (8 mm)", ref, CLASS_OTHER);
    floor_fill(200); disc(45, 90);         check("big disc (45 mm, bottle cap+)", ref, CLASS_OTHER);
    floor_fill(200);                       check("empty pit, no reference", NULL, CLASS_NONE);

    printf("%s\n", failures ? "FAILED" : "ALL PASS");
    return failures;
}
