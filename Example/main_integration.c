/* Copy the relevant parts into the CubeMX-generated Core/Src/main.c. */
#include "esp_at.h"
#include "main.h"

#include <string.h>

extern UART_HandleTypeDef huart1;

static ESP_AT_Handle esp;
static volatile size_t received_bytes;

static void esp_line(const char *line, void *context)
{
    (void)line;
    (void)context;
    /* Handle WIFI DISCONNECT, CLOSED, etc. here. Do not block. */
}

static void esp_data(const uint8_t *data, size_t length, void *context)
{
    (void)data;
    (void)context;
    received_bytes += length;
    /* Copy or consume data here; the pointer is only valid in this callback. */
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    ESP_AT_RxCpltCallback(&esp, huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    ESP_AT_ErrorCallback(&esp, huart);
}

/* Call once after MX_USART1_UART_Init(). */
static ESP_AT_Status app_esp_start(void)
{
    static const uint8_t request[] =
        "GET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n";
    ESP_AT_Status status;

    ESP_AT_Init(&esp, &huart1, esp_line, esp_data, NULL);
    status = ESP_AT_StartReceive(&esp);
    if (status != ESP_AT_OK) return status;
    status = ESP_AT_Command(&esp, "ATE0", 1000U);
    if (status != ESP_AT_OK) return status;
    status = ESP_AT_SetStationMode(&esp, 1000U);
    if (status != ESP_AT_OK) return status;
    status = ESP_AT_JoinAP(&esp, "YOUR_SSID", "YOUR_PASSWORD", 20000U);
    if (status != ESP_AT_OK) return status;
    status = ESP_AT_TcpConnect(&esp, "example.com", 80U, 10000U);
    if (status != ESP_AT_OK) return status;
    return ESP_AT_Send(&esp, request, sizeof(request) - 1U, 10000U);
}

/* Call continuously in the generated while (1) loop. */
static void app_esp_loop(void)
{
    (void)ESP_AT_Process(&esp);
}
