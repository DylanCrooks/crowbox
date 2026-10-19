#include "vision_protocol.h"

uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];

        for (int bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;
            } else { 
                crc <<= 1;
            }
        }
    }
    return crc;
}

bool msg_pack(const UART_msg_t *msg, uint8_t out[6] ) {

    if (msg == NULL || out == NULL) {return false;}
    

    out[0] = START_BYTE;
    out[1] = msg->type;
    out[2] = msg->ID;
    out[3] = msg->arg0;
    out[4] = msg->arg1;
    uint8_t CRC = crc8(&out[1], 4);
    out[5] = CRC;
    
    return true;
}

bool msg_unpack(const uint8_t in[6], UART_msg_t *m) {
    if (in[0] != START_BYTE) {return false;}
    if (crc8(&in[1], 4) != in[5]) {return false;}

    m->type = in[1];
    m->ID = in[2];
    m->arg0 = in[3];
    m->arg1 = in[4];
    
    return true;
}