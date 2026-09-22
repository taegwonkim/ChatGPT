#include "esp_at.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static ESP_AT_Handle *active_handle;
static uint32_t ticks;
static char transmitted[512];
static size_t transmitted_length;
static uint8_t received[64];
static size_t received_length;

static void inject(const char *text)
{
    while (*text != '\0') {
        active_handle->rx_byte = (uint8_t)*text++;
        ESP_AT_RxCpltCallback(active_handle, active_handle->uart);
    }
}

uint32_t HAL_GetTick(void)
{
    return ticks++;
}

HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *uart, uint8_t *data,
                                     uint16_t length)
{
    (void)uart;
    (void)data;
    return length == 1U ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *uart, uint8_t *data,
                                    uint16_t length, uint32_t timeout)
{
    (void)uart;
    (void)timeout;
    assert(transmitted_length + length < sizeof(transmitted));
    memcpy(transmitted + transmitted_length, data, length);
    transmitted_length += length;
    transmitted[transmitted_length] = '\0';

    if (length == 2U && memcmp(data, "\r\n", 2U) == 0) {
        inject("\r\nOK\r\n");
    } else if (strncmp((const char *)data, "AT+CIPSEND=", 11U) == 0) {
        inject("\r\nOK\r\n> ");
    } else if (length == 4U && memcmp(data, "PING", 4U) == 0) {
        inject("\r\nSEND OK\r\n");
    }
    return HAL_OK;
}

static void on_data(const uint8_t *data, size_t length, void *context)
{
    (void)context;
    assert(received_length + length <= sizeof(received));
    memcpy(received + received_length, data, length);
    received_length += length;
}

int main(void)
{
    UART_HandleTypeDef uart = {0};
    ESP_AT_Handle handle;

    active_handle = &handle;
    ESP_AT_Init(&handle, &uart, NULL, on_data, NULL);
    assert(ESP_AT_StartReceive(&handle) == ESP_AT_OK);
    assert(ESP_AT_Command(&handle, "AT", 100U) == ESP_AT_OK);
    assert(strcmp(transmitted, "AT\r\n") == 0);

    transmitted_length = 0U;
    assert(ESP_AT_Send(&handle, (const uint8_t *)"PING", 4U, 100U) ==
           ESP_AT_OK);
    assert(strcmp(transmitted, "AT+CIPSEND=4\r\nPING") == 0);

    inject("\r\n+IPD,5:hello\r\n");
    assert(ESP_AT_Process(&handle) == ESP_AT_OK);
    assert(received_length == 5U);
    assert(memcmp(received, "hello", 5U) == 0);

    puts("esp_at tests passed");
    return 0;
}
