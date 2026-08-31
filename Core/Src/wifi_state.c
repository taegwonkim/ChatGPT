#include "wifi_state.h"
void WifiMachine_Init(WifiMachine *m, uint32_t now) { m->state=WIFI_RESET; m->failures=0; m->deadline_ms=now; }
uint32_t WifiMachine_BackoffMs(uint8_t n, uint32_t entropy) {
  uint32_t base = 1000u << (n > 4u ? 4u : n);
  if (base > 30000u) base = 30000u;
  return base + entropy % 251u;
}
void WifiMachine_Succeeded(WifiMachine *m, uint32_t now) { m->failures=0; m->state=WIFI_ONLINE; m->deadline_ms=now+10000u; }
void WifiMachine_Failed(WifiMachine *m, uint32_t now, uint32_t entropy) { if(m->failures<255u)m->failures++; m->state=WIFI_BACKOFF; m->deadline_ms=now+WifiMachine_BackoffMs(m->failures-1u,entropy); }
