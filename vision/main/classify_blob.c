// Classifier v1: classic CV (background subtraction, threshold, blobs, roundness, size).
// Keep this file free of ESP-IDF code so it also compiles and tests on a laptop.
//
// Pipeline, on a small grayscale image (320x240 from preprocess_gray):
//   1. Background: the empty-pit reference frame, shifted by the difference in overall
//      brightness (auto-exposure drifts between shots). No reference yet: the floor's
//      typical (median) brightness everywhere.
//   2. Mask: pixels that differ from the background by more than BLOB_DIFF_THRESHOLD.
//   3. Blobs: 4-connected groups of mask pixels (flood fill). Tiny ones are noise; ones
//      touching the image edge are cut off, so their shape and size can't be trusted.
//   4. Largest remaining blob -> size (equivalent diameter, in mm) and roundness (overlap
//      with a disc of the same area). Coin = coin-sized and round.
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "classify.h"

// ---- Tuning: calibrate on the real box ----
#define BLOB_DIFF_THRESHOLD     30      // gray levels (0-255) that count as "not background"
#define BLOB_MIN_AREA_PX        40      // smaller blobs are sensor noise / grit
// Pixel size on the pit floor at 320x240. Estimate: OV5647 binned 1280x960 (~3.58 mm wide
// sensor area), 3.6 mm lens, ~300 mm away -> ~295 mm across 320 px. Measure: photograph a
// ruler on the floor and divide its length in mm by its length in pixels.
#define BLOB_MM_PER_PX          0.92f
// US coins: dime 17.9 mm, penny 19.05, nickel 21.2, quarter 24.26.
#define COIN_MIN_DIAMETER_MM    16.0f
#define COIN_MAX_DIAMETER_MM    27.0f
// Disc overlap (IoU with a same-area disc at the blob's center): circle ~0.95-1.0,
// square ~0.83, 2:1 ellipse ~0.7.
#define COIN_MIN_ROUNDNESS      0.88f

#define CONF_NONE               90      // nothing there: background subtraction is reliable
#define CONF_OTHER              70      // something there that isn't coin-shaped/sized
#define CONF_COIN_MIN           60      // at COIN_MIN_ROUNDNESS
#define CONF_COIN_MAX           99      // at a perfect disc

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int s_w, s_h;
static uint8_t *s_mask;     // 1 = differs from background and not yet visited
static uint32_t *s_queue;   // flood-fill queue; after a fill it holds that blob's pixels

typedef struct {
    uint32_t area;
    float roundness;
} blob_t;

bool classify_blob_init(int w, int h)
{
    s_w = w;
    s_h = h;
    s_mask = malloc((size_t)w * h);
    s_queue = malloc((size_t)w * h * sizeof(uint32_t));
    return s_mask != NULL && s_queue != NULL;
}

static uint8_t median_of(const uint8_t *img, int n)
{
    uint32_t hist[256] = {0};
    for (int i = 0; i < n; i++) {
        hist[img[i]]++;
    }
    uint32_t seen = 0;
    for (int v = 0; v < 256; v++) {
        seen += hist[v];
        if (seen * 2 >= (uint32_t)n) {
            return (uint8_t)v;
        }
    }
    return 255;
}

static void build_mask(const uint8_t *gray, const uint8_t *ref, int n)
{
    int gray_median = median_of(gray, n);
    int shift = ref ? gray_median - median_of(ref, n) : 0;

    for (int i = 0; i < n; i++) {
        int bg = ref ? ref[i] + shift : gray_median;
        s_mask[i] = abs(gray[i] - bg) > BLOB_DIFF_THRESHOLD;
    }
}

// Flood-fills the blob containing `start`, clearing its mask pixels and leaving its pixel
// indices in s_queue[0 .. area). Returns the area; *touches_edge reports whether any of its
// pixels lie on the image border.
static uint32_t fill_blob(uint32_t start, bool *touches_edge)
{
    uint32_t head = 0;
    uint32_t tail = 0;
    *touches_edge = false;

    s_mask[start] = 0;
    s_queue[tail++] = start;

    while (head < tail) {
        uint32_t p = s_queue[head++];
        int x = p % s_w;
        int y = p / s_w;

        if (x == 0 || y == 0 || x == s_w - 1 || y == s_h - 1) {
            *touches_edge = true;
        }
        // 4-connected neighbors: left, right, up, down.
        if (x > 0 && s_mask[p - 1]) {
            s_mask[p - 1] = 0;
            s_queue[tail++] = p - 1;
        }
        if (x < s_w - 1 && s_mask[p + 1]) {
            s_mask[p + 1] = 0;
            s_queue[tail++] = p + 1;
        }
        if (y > 0 && s_mask[p - s_w]) {
            s_mask[p - s_w] = 0;
            s_queue[tail++] = p - s_w;
        }
        if (y < s_h - 1 && s_mask[p + s_w]) {
            s_mask[p + s_w] = 0;
            s_queue[tail++] = p + s_w;
        }
    }
    return tail;
}

// Overlap of the blob in s_queue[0 .. area) with a disc of equal area at its centroid,
// as intersection over union: 1.0 = perfect disc.
static float roundness_of(uint32_t area)
{
    double sx = 0, sy = 0;
    for (uint32_t i = 0; i < area; i++) {
        sx += s_queue[i] % s_w;
        sy += s_queue[i] / s_w;
    }
    double cx = sx / area;
    double cy = sy / area;
    double r2 = area / M_PI;

    uint32_t inside = 0;
    for (uint32_t i = 0; i < area; i++) {
        double dx = (double)(s_queue[i] % s_w) - cx;
        double dy = (double)(s_queue[i] / s_w) - cy;
        if (dx * dx + dy * dy <= r2) {
            inside++;
        }
    }
    // Both shapes have the same area, so union = 2 * area - intersection.
    return (float)inside / (float)(2 * area - inside);
}

bool classify_blob(const uint8_t *gray, const uint8_t *ref, classify_result_t *out)
{
    if (s_mask == NULL || s_queue == NULL || gray == NULL || out == NULL) {
        return false;
    }
    int n = s_w * s_h;
    build_mask(gray, ref, n);

    blob_t best = {0};
    for (int i = 0; i < n; i++) {
        if (!s_mask[i]) {
            continue;
        }
        bool touches_edge;
        uint32_t area = fill_blob((uint32_t)i, &touches_edge);
        if (area < BLOB_MIN_AREA_PX || touches_edge || area <= best.area) {
            continue;
        }
        best.area = area;
        best.roundness = roundness_of(area);    // s_queue still holds this blob's pixels
    }

    if (best.area == 0) {
        out->cls = CLASS_NONE;
        out->confidence = CONF_NONE;
        return true;
    }

    float diameter_mm = 2.0f * sqrtf(best.area / (float)M_PI) * BLOB_MM_PER_PX;
    bool coin_sized = diameter_mm >= COIN_MIN_DIAMETER_MM && diameter_mm <= COIN_MAX_DIAMETER_MM;
    bool round = best.roundness >= COIN_MIN_ROUNDNESS;

    if (coin_sized && round) {
        float t = (best.roundness - COIN_MIN_ROUNDNESS) / (1.0f - COIN_MIN_ROUNDNESS);
        if (t > 1.0f) {
            t = 1.0f;
        }
        out->cls = CLASS_COIN;
        out->confidence = (uint8_t)(CONF_COIN_MIN + t * (CONF_COIN_MAX - CONF_COIN_MIN));
    } else {
        out->cls = CLASS_OTHER;
        out->confidence = CONF_OTHER;
    }
    return true;
}
