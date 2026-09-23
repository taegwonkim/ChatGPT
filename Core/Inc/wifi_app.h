#ifndef WIFI_APP_H
#define WIFI_APP_H

#include "esp32_at.h"

typedef struct {
    const char *ssid;
    const char *password;
    const char *server_ip;
    uint16_t server_port;
    bool dhcp_enabled;
    const char *static_ip;
    const char *gateway;
    const char *netmask;
    uint32_t health_check_ms;
    uint32_t retry_ms;
} WifiAppConfig;

typedef enum {
    WIFI_APP_INIT,
    WIFI_APP_AP_CONNECT,
    WIFI_APP_SERVER_CONNECT,
    WIFI_APP_ONLINE,
    WIFI_APP_RETRY_WAIT
} WifiAppState;

typedef struct {
    Esp32At esp;
    WifiAppConfig config;
    WifiAppState state;
    WifiAppState retry_target;
    uint32_t next_action_tick;
    char mac[ESP32_AT_MAC_STRING_SIZE];
} WifiApp;

void WifiApp_Init(WifiApp *app, UART_HandleTypeDef *uart,
                  const WifiAppConfig *config);
void WifiApp_Process(WifiApp *app);
bool WifiApp_IsOnline(const WifiApp *app);

#endif
