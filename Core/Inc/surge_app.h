#ifndef SURGE_APP_H
#define SURGE_APP_H
#include <stddef.h>
#include <stdint.h>
void Surge_AppInit(void);
void Surge_RtosInit(void);
void Surge_FpgaTriggerFromISR(void);
void Surge_FpgaRxBytesFromISR(const uint8_t *data, size_t size);
void Surge_PcRxBytesFromISR(const uint8_t *data, size_t size);
#endif
