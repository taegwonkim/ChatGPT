/* CubeMX가 생성한 main.c의 USER CODE 영역에 필요한 부분만 옮기십시오. */
#include "esp32_at.h"
#include <stdio.h>
#include <string.h>

#define WIFI_SSID      "YOUR_AP_SSID"
#define WIFI_PASSWORD  "YOUR_AP_PASSWORD"
#define SERVER_IP      "192.168.0.10"
#define SERVER_PORT    5000U

extern UART_HandleTypeDef huart1; /* ESP32-C3 */
extern UART_HandleTypeDef huart2; /* 선택: PC debug console */

static void received(const uint8_t *data, size_t length)
{
    /* callback은 main context에서 실행됩니다. binary 안전하게 length를 사용합니다. */
    (void)HAL_UART_Transmit(&huart2, (uint8_t *)"RX: ", 4U, 100U);
    (void)HAL_UART_Transmit(&huart2, (uint8_t *)data, (uint16_t)length, 1000U);
    (void)HAL_UART_Transmit(&huart2, (uint8_t *)"\r\n", 2U, 100U);
}

static void state_changed(ESP_AT_State state)
{
    char message[32];
    int length = snprintf(message, sizeof(message), "ESP state=%d\r\n", (int)state);
    (void)HAL_UART_Transmit(&huart2, (uint8_t *)message, (uint16_t)length, 100U);
}

/* main(): MX_USARTx_UART_Init() 호출 이후 */
static void App_Init(void)
{
    const ESP_AT_Config config = {
        .uart = &huart1,
        .ssid = WIFI_SSID,
        .password = WIFI_PASSWORD,
        .server_host = SERVER_IP,
        .server_port = SERVER_PORT,
        .on_data = received,
        .on_state = state_changed,
    };
    ESP_AT_Init(&config);
    ESP_AT_Start();
}

/* while (1) 안에서 호출하는 예. delay로 Process를 오래 막지 마십시오. */
static void App_Loop(void)
{
    static uint32_t next_send;
    static unsigned int sequence;
    char message[64];

    ESP_AT_Process();
    if (ESP_AT_IsOnline() && (int32_t)(HAL_GetTick() - next_send) >= 0) {
        int length = snprintf(message, sizeof(message), "STM32 seq=%u tick=%lu\n",
                              sequence, (unsigned long)HAL_GetTick());
        if (ESP_AT_Send((const uint8_t *)message, (size_t)length)) {
            sequence++;
            next_send = HAL_GetTick() + 5000U;
        }
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    ESP_AT_RxCpltCallback(huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    ESP_AT_UartErrorCallback(huart);
}

/* 실제 main()에서:
 *   App_Init();
 *   while (1) { App_Loop(); }
 */
