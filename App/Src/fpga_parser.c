#include "fpga_parser.h"
#include "app_config.h"
#include <string.h>

void fpga_parser_reset(FpgaParser *p) { p->used = 0u; p->errors = 0u; }
static uint16_t le16(const uint8_t *p) { return (uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8); }
static uint32_t le32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

bool fpga_parser_push(FpgaParser *p, uint8_t byte, FpgaFrame *frame) {
    if (p->used == 0u && byte != 0xA5u) return false;
    if (p->used == 1u && byte != 0x5Au) { p->used = byte == 0xA5u ? 1u : 0u; ++p->errors; return false; }
    p->buffer[p->used++] = byte;
    if (p->used < 4u) return false;
    const uint16_t length = le16(&p->buffer[2]);
    if (length > FPGA_PAYLOAD_MAX) { p->used = 0u; ++p->errors; return false; }
    const size_t total = 8u + length + 4u;
    if (p->used < total) return false;
    const uint32_t expected = le32(&p->buffer[8u + length]);
    if (app_crc32(p->buffer, 8u + length) != expected) { p->used = 0u; ++p->errors; return false; }
    frame->length = length;
    frame->sequence = le32(&p->buffer[4]);
    memcpy(frame->payload, &p->buffer[8], length);
    p->used = 0u;
    return true;
}
