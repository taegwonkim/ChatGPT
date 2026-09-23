#ifndef ESP32_AT_H
#define ESP32_AT_H

#include "stm32l5xx_hal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESP32_AT_RESPONSE_SIZE 768U
#define ESP32_AT_MAC_STRING_SIZE 18U

typedef enum {
    ESP32_AT_OK = 0,
    ESP32_AT_TIMEOUT,
    ESP32_AT_ERROR,
    ESP32_AT_OVERFLOW,
    ESP32_AT_BAD_ARGUMENT,
    ESP32_AT_HAL_ERROR
} Esp32AtStatus;

typedef struct {
    UART_HandleTypeDef *uart;
    char response[ESP32_AT_RESPONSE_SIZE];
    size_t response_length;
} Esp32At;

void Esp32At_Init(Esp32At *dev, UART_HandleTypeDef *uart);
Esp32AtStatus Esp32At_Begin(Esp32At *dev, char mac[ESP32_AT_MAC_STRING_SIZE]);
Esp32AtStatus Esp32At_SetDhcp(Esp32At *dev, bool enable,
                              const char *ip, const char *gateway,
                              const char *netmask);
Esp32AtStatus Esp32At_JoinAp(Esp32At *dev, const char *ssid,
                             const char *password);
Esp32AtStatus Esp32At_OpenTcp(Esp32At *dev, const char *host, uint16_t port);
bool Esp32At_IsApConnected(Esp32At *dev);
bool Esp32At_IsTcpConnected(Esp32At *dev);
Esp32AtStatus Esp32At_CloseTcp(Esp32At *dev);
Esp32AtStatus Esp32At_Command(Esp32At *dev, const char *command,
                              uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
