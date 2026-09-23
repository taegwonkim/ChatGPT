#ifndef ESP_AT_H
#define ESP_AT_H

#include "main.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ESP_AT_RX_BUFFER_SIZE
#define ESP_AT_RX_BUFFER_SIZE 512U
#endif

#ifndef ESP_AT_LINE_BUFFER_SIZE
#define ESP_AT_LINE_BUFFER_SIZE 256U
#endif

#ifndef ESP_AT_IPD_CHUNK_SIZE
#define ESP_AT_IPD_CHUNK_SIZE 128U
#endif

typedef enum {
    ESP_AT_OK = 0,
    ESP_AT_ERROR,
    ESP_AT_TIMEOUT,
    ESP_AT_BUSY,
    ESP_AT_OVERFLOW,
    ESP_AT_INVALID_ARG,
    ESP_AT_HAL_ERROR
} ESP_AT_Status;

typedef void (*ESP_AT_LineCallback)(const char *line, void *context);
typedef void (*ESP_AT_DataCallback)(const uint8_t *data, size_t length,
                                    void *context);

typedef struct {
    UART_HandleTypeDef *uart;
    ESP_AT_LineCallback line_callback;
    ESP_AT_DataCallback data_callback;
    void *callback_context;

    volatile uint16_t rx_head;
    volatile uint16_t rx_tail;
    volatile bool rx_overflow;
    uint8_t rx_byte;
    uint8_t rx_buffer[ESP_AT_RX_BUFFER_SIZE];

    char line[ESP_AT_LINE_BUFFER_SIZE];
    size_t line_length;
    size_t ipd_remaining;
    uint8_t ipd_chunk[ESP_AT_IPD_CHUNK_SIZE];
    size_t ipd_chunk_length;

    volatile bool command_active;
    volatile bool response_ok;
    volatile bool response_error;
    volatile bool prompt_received;
} ESP_AT_Handle;

void ESP_AT_Init(ESP_AT_Handle *handle, UART_HandleTypeDef *uart,
                 ESP_AT_LineCallback line_callback,
                 ESP_AT_DataCallback data_callback, void *context);
ESP_AT_Status ESP_AT_StartReceive(ESP_AT_Handle *handle);
void ESP_AT_RxCpltCallback(ESP_AT_Handle *handle,
                           UART_HandleTypeDef *uart);
void ESP_AT_ErrorCallback(ESP_AT_Handle *handle, UART_HandleTypeDef *uart);
ESP_AT_Status ESP_AT_Process(ESP_AT_Handle *handle);

ESP_AT_Status ESP_AT_Command(ESP_AT_Handle *handle, const char *command,
                            uint32_t timeout_ms);
ESP_AT_Status ESP_AT_Reset(ESP_AT_Handle *handle, uint32_t timeout_ms);
ESP_AT_Status ESP_AT_SetStationMode(ESP_AT_Handle *handle,
                                    uint32_t timeout_ms);
ESP_AT_Status ESP_AT_JoinAP(ESP_AT_Handle *handle, const char *ssid,
                            const char *password, uint32_t timeout_ms);
ESP_AT_Status ESP_AT_TcpConnect(ESP_AT_Handle *handle, const char *host,
                                uint16_t port, uint32_t timeout_ms);
ESP_AT_Status ESP_AT_Send(ESP_AT_Handle *handle, const uint8_t *data,
                          size_t length, uint32_t timeout_ms);
ESP_AT_Status ESP_AT_Close(ESP_AT_Handle *handle, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
