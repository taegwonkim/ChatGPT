#include "fpga_protocol.h"
#include "surge_config.h"
#include <string.h>

static uint16_t le16(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
static uint32_t le32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

void FpgaParser_Init(FpgaParser *p) { memset(p, 0, sizeof(*p)); }

bool FpgaParser_Push(FpgaParser *p, uint8_t b, FpgaFrameView *out) {
  if (p->state == 0u) { p->state = (b == 0xA5u) ? 1u : 0u; return false; }
  if (p->state == 1u) { p->state = (b == 0x5Au) ? 2u : (b == 0xA5u); return false; }
  if (p->state == 2u) {
    p->header[p->header_used++] = b;
    if (p->header_used < sizeof(p->header)) return false;
    uint16_t n = le16(&p->header[2]);
    if (p->header[0] != 1u || p->header[1] != FPGA_CHANNELS || n == 0u || n > FPGA_MAX_SAMPLES) {
      FpgaParser_Init(p); return false;
    }
    p->payload_expected = (size_t)FPGA_CHANNELS * n * 2u + FPGA_CHANNELS * 2u;
    p->state = 3u; return false;
  }
  if (p->state == 3u) {
    p->payload[p->payload_used++] = b;
    if (p->payload_used == p->payload_expected) p->state = 4u;
    return false;
  }
  p->crc[p->crc_used++] = b;
  if (p->crc_used < 4u) return false;
  uint8_t all[sizeof(p->header) + FPGA_MAX_PAYLOAD];
  memcpy(all, p->header, sizeof(p->header));
  memcpy(all + sizeof(p->header), p->payload, p->payload_expected);
  bool valid = Surge_Crc32(all, sizeof(p->header) + p->payload_expected) == le32(p->crc);
  if (valid && out != NULL) {
    out->samples_per_channel = le16(&p->header[2]);
    out->sequence = le32(&p->header[4]);
    out->payload = p->payload;
    out->payload_size = p->payload_expected;
  }
  p->state = p->header_used = p->payload_used = p->crc_used = 0u;
  return valid;
}
