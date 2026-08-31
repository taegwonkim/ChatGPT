#ifndef WIFI_STATE_H
#define WIFI_STATE_H
#include <stdbool.h>
#include <stdint.h>
typedef enum { WIFI_RESET, WIFI_AT_SYNC, WIFI_SET_MODE, WIFI_NET_CONFIG, WIFI_JOIN_AP, WIFI_OPEN_SOCKET, WIFI_ONLINE, WIFI_BACKOFF } WifiState;
typedef struct { WifiState state; uint8_t failures; uint32_t deadline_ms; } WifiMachine;
void WifiMachine_Init(WifiMachine *m, uint32_t now);
uint32_t WifiMachine_BackoffMs(uint8_t failures, uint32_t entropy);
void WifiMachine_Succeeded(WifiMachine *m, uint32_t now);
void WifiMachine_Failed(WifiMachine *m, uint32_t now, uint32_t entropy);
#endif
