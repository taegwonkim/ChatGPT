#ifndef FPGA_PARSER_H
#define FPGA_PARSER_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define FPGA_PAYLOAD_MAX 480u
typedef struct { uint32_t sequence; uint16_t length; uint8_t payload[FPGA_PAYLOAD_MAX]; } FpgaFrame;
typedef struct { uint8_t buffer[2u + 2u + 4u + FPGA_PAYLOAD_MAX + 4u]; size_t used; uint32_t errors; } FpgaParser;
void fpga_parser_reset(FpgaParser *parser);
bool fpga_parser_push(FpgaParser *parser, uint8_t byte, FpgaFrame *frame);
#endif
