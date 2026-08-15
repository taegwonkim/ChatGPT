#pragma once

#include <stdint.h>

#define APP_LINE_MAX             256U
#define APP_MEAS_QUEUE_DEPTH     16U
#define APP_TX_QUEUE_DEPTH       16U
#define APP_WIFI_SSID_MAX        32U
#define APP_WIFI_PASSWORD_MAX    64U
#define APP_SERVER_IP_MAX        45U
#define APP_FLASH_SECTOR_SIZE    4096U
#define APP_CFG_SLOT_A           0x000000U
#define APP_CFG_SLOT_B           0x001000U
#define APP_FLASH_PAGE_SIZE      256U

typedef struct {
    char ssid[APP_WIFI_SSID_MAX + 1U];
    char password[APP_WIFI_PASSWORD_MAX + 1U];
    char server_ip[APP_SERVER_IP_MAX + 1U];
    uint16_t server_port;
    uint8_t dhcp;
} AppWifiConfig;

typedef struct {
    uint32_t sequence;
    int32_t value;
    uint32_t timestamp_ms;
} AppMeasurement;
