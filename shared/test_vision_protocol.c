// Throwaway laptop test for the shared protocol code. Not part of either firmware build.
// Build and run from shared/:  gcc -Wall -Wextra test_vision_protocol.c vision_protocol.c -o test && ./test
#include <stdio.h>
#include <string.h>
#include "vision_protocol.h"

int main(void)
{
    int failures = 0;

    // 1. CRC-8/SMBUS check value
    uint8_t crc = crc8((const uint8_t *)"123456789", 9);
    printf("crc8(\"123456789\") = 0x%02X (want 0xF4)\n", crc);
    if (crc != 0xF4) failures++;

    // 2. Round trip
    UART_msg_t in = { .type = MSG_RESULT, .ID = 42, .arg0 = 3, .arg1 = 87 };
    UART_msg_t out;
    uint8_t buf[6];
    msg_pack(&in, buf);
    printf("packed: %02X %02X %02X %02X %02X %02X\n",
           buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
    bool ok = msg_unpack(buf, &out);
    printf("round trip: %s\n", ok && memcmp(&in, &out, sizeof in) == 0 ? "PASS" : "FAIL");
    if (!ok || memcmp(&in, &out, sizeof in) != 0) failures++;

    // 3. Every single-bit flip in the frame must be rejected
    int missed = 0;
    for (int byte = 0; byte < 6; byte++) {
        for (int bit = 0; bit < 8; bit++) {
            uint8_t bad[6];
            memcpy(bad, buf, 6);
            bad[byte] ^= (uint8_t)(1u << bit);
            if (msg_unpack(bad, &out)) missed++;
        }
    }
    printf("single-bit flips accepted: %d of 48 (want 0)\n", missed);
    if (missed) failures++;

    printf("%s\n", failures ? "FAILED" : "ALL PASS");
    return failures;
}
