#ifndef ESP32_AT_H
#define ESP32_AT_H

#include "main.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef ESP_AT_RX_BUFFER_SIZE
#define ESP_AT_RX_BUFFER_SIZE 1024U
#endif

#ifndef ESP_AT_MAX_PAYLOAD
#define ESP_AT_MAX_PAYLOAD 512U
#endif

typedef enum {
    ESP_AT_STOPPED,
    ESP_AT_SYNC,
    ESP_AT_CONFIG,
    ESP_AT_JOIN_AP,
    ESP_AT_CONNECT_TCP,
    ESP_AT_ONLINE,
    ESP_AT_BACKOFF
} ESP_AT_State;

typedef void (*ESP_AT_DataCallback)(const uint8_t *data, size_t length);
typedef void (*ESP_AT_StateCallback)(ESP_AT_State state);

typedef struct {
    UART_HandleTypeDef *uart;
    const char *ssid;
    const char *password;
    const char *server_host;
    uint16_t server_port;
    ESP_AT_DataCallback on_data;
    ESP_AT_StateCallback on_state;
} ESP_AT_Config;

void ESP_AT_Init(const ESP_AT_Config *config);
void ESP_AT_Start(void);
void ESP_AT_Process(void);
bool ESP_AT_Send(const uint8_t *data, size_t length);
bool ESP_AT_IsOnline(void);
ESP_AT_State ESP_AT_GetState(void);

/* HAL_UART_RxCpltCallback()에서 호출합니다. */
void ESP_AT_RxCpltCallback(UART_HandleTypeDef *huart);
/* HAL_UART_ErrorCallback()에서 호출합니다. */
void ESP_AT_UartErrorCallback(UART_HandleTypeDef *huart);

#endif
