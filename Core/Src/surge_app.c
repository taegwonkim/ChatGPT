#include "surge_app.h"

/* Board/RTOS integration seam. See docs/INTEGRATION.md. Keeping HAL handles out of
 * protocol modules makes their bounds/CRC logic host-testable. */
__attribute__((weak)) void Surge_PortInit(void) {}
__attribute__((weak)) void Surge_PortCreateRtosObjects(void) {}
__attribute__((weak)) void Surge_PortNotifyFpgaTriggerFromISR(void) {}
__attribute__((weak)) void Surge_PortWriteFpgaStreamFromISR(const uint8_t *d, size_t n) {(void)d;(void)n;}
__attribute__((weak)) void Surge_PortWritePcStreamFromISR(const uint8_t *d, size_t n) {(void)d;(void)n;}

void Surge_AppInit(void) { Surge_PortInit(); }
void Surge_RtosInit(void) { Surge_PortCreateRtosObjects(); }
void Surge_FpgaTriggerFromISR(void) { Surge_PortNotifyFpgaTriggerFromISR(); }
void Surge_FpgaRxBytesFromISR(const uint8_t *d, size_t n) { Surge_PortWriteFpgaStreamFromISR(d,n); }
void Surge_PcRxBytesFromISR(const uint8_t *d, size_t n) { Surge_PortWritePcStreamFromISR(d,n); }
