// Preprocessing: crop the frame to the pit, resize for the model.
#include "preprocess.h"

// Byte order of each RGB565 pixel in the camera buffer. 0 = little-endian uint16
// (assumed ISP default). If the gray images come out as noise on the real board, set to 1.
#define PREPROCESS_RGB565_SWAP_BYTES 0

static inline uint16_t pixel_at(const uint8_t *rgb565, int i)
{
    uint8_t lo = rgb565[2 * i];
    uint8_t hi = rgb565[2 * i + 1];
#if PREPROCESS_RGB565_SWAP_BYTES
    return (uint16_t)((lo << 8) | hi);
#else
    return (uint16_t)((hi << 8) | lo);
#endif
}

// RGB565 packs R in bits 15-11, G in 10-5, B in 4-0. Widen each to 8 bits by
// repeating its top bits into the new low bits, so 31 -> 255 rather than 248.
static inline void unpack(uint16_t p, uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint8_t r5 = (p >> 11) & 0x1F;
    uint8_t g6 = (p >> 5) & 0x3F;
    uint8_t b5 = p & 0x1F;
    *r = (uint8_t)((r5 << 3) | (r5 >> 2));
    *g = (uint8_t)((g6 << 2) | (g6 >> 4));
    *b = (uint8_t)((b5 << 3) | (b5 >> 2));
}

// Luma weights (ITU-R BT.601) scaled to sum to 256: 0.299, 0.587, 0.114.
static inline uint8_t gray_of(uint16_t p)
{
    uint8_t r, g, b;
    unpack(p, &r, &g, &b);
    return (uint8_t)((77 * r + 150 * g + 29 * b) >> 8);
}

void preprocess_gray(const uint8_t *rgb565, int w, int h, int scale, uint8_t *out)
{
    int ow = w / scale;
    int oh = h / scale;
    int block = scale * scale;

    for (int oy = 0; oy < oh; oy++) {
        for (int ox = 0; ox < ow; ox++) {
            uint32_t sum = 0;
            for (int dy = 0; dy < scale; dy++) {
                int row = (oy * scale + dy) * w + ox * scale;
                for (int dx = 0; dx < scale; dx++) {
                    sum += gray_of(pixel_at(rgb565, row + dx));
                }
            }
            out[oy * ow + ox] = (uint8_t)(sum / block);
        }
    }
}

void preprocess_model_input(const uint8_t *rgb565, int w, int h, int size, uint8_t *out_rgb)
{
    int side = (w < h) ? w : h;
    int x0 = (w - side) / 2;
    int y0 = (h - side) / 2;

    // Nearest-neighbor: each output pixel takes the input pixel at its scaled position.
    for (int oy = 0; oy < size; oy++) {
        int sy = y0 + oy * side / size;
        for (int ox = 0; ox < size; ox++) {
            int sx = x0 + ox * side / size;
            uint8_t *o = &out_rgb[(oy * size + ox) * 3];
            unpack(pixel_at(rgb565, sy * w + sx), &o[0], &o[1], &o[2]);
        }
    }
}
