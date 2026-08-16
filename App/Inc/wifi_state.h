#ifndef WIFI_STATE_H
#define WIFI_STATE_H
#include <stdbool.h>
#include <stdint.h>
typedef enum { WIFI_RESET, WIFI_WAIT_READY, WIFI_CONFIG_IP, WIFI_JOIN_AP, WIFI_OPEN_TCP, WIFI_ONLINE, WIFI_BACKOFF } WifiState;
typedef struct { WifiState state; uint8_t attempts; uint32_t deadline_ms; } WifiMachine;
void wifi_init(WifiMachine *wifi, uint32_t now_ms);
void wifi_disconnected(WifiMachine *wifi, uint32_t now_ms);
bool wifi_backoff_expired(WifiMachine *wifi, uint32_t now_ms);
uint32_t wifi_backoff_seconds(uint8_t attempts);
#endif
