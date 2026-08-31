#ifndef FPGA_PROTOCOL_H
#define FPGA_PROTOCOL_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FPGA_CHANNELS 6u
#define FPGA_MAX_SAMPLES 256u
#define FPGA_MAX_PAYLOAD (FPGA_CHANNELS * FPGA_MAX_SAMPLES * 2u + FPGA_CHANNELS * 2u)

typedef struct {
  uint8_t state;
  uint8_t header[8];
  size_t header_used;
  uint8_t payload[FPGA_MAX_PAYLOAD];
  size_t payload_used, payload_expected;
  uint8_t crc[4];
  size_t crc_used;
} FpgaParser;

typedef struct {
  uint32_t sequence;
  uint16_t samples_per_channel;
  const uint8_t *payload;
  size_t payload_size;
} FpgaFrameView;

void FpgaParser_Init(FpgaParser *parser);
bool FpgaParser_Push(FpgaParser *parser, uint8_t byte, FpgaFrameView *frame);
#endif
