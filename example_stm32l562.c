/* STM32CubeL5 프로젝트에 넣을 수 있는 HAL 어댑터 예제입니다. */
#include "esp32c3_at.h"
#include "main.h"

#include <string.h>

extern UART_HandleTypeDef huart1; /* ESP32-C3: 115200, 8-N-1 */
static esp_at_t wifi;

static size_t uart_write(const uint8_t *data, size_t length, void *user)
{
    UART_HandleTypeDef *uart = user;
    return HAL_UART_Transmit(uart, (uint8_t *)data, (uint16_t)length, 1000U) ==
                   HAL_OK
               ? length
               : 0U;
}

static size_t uart_read(uint8_t *data, size_t capacity, uint32_t timeout_ms,
                        void *user)
{
    UART_HandleTypeDef *uart = user;
    size_t count = 0U;
    while (count < capacity) {
        uint32_t timeout = count == 0U ? timeout_ms : 1U;
        if (HAL_UART_Receive(uart, &data[count], 1U, timeout) != HAL_OK) {
            break;
        }
        ++count;
    }
    return count;
}

static uint32_t get_tick(void *user)
{
    (void)user;
    return HAL_GetTick();
}

static void delay_ms(uint32_t ms, void *user)
{
    (void)user;
    HAL_Delay(ms);
}

bool App_WifiInit(void)
{
    const esp_at_config_t config = {
        .ssid = "MY_AP",
        .password = "MY_PASSWORD",
        .dhcp = false,
        .local_ip = "192.168.0.50",
        .gateway = "192.168.0.1",
        .netmask = "255.255.255.0",
        .server_ip = "192.168.0.100",
        .server_port = 5000U,
        .protocol = ESP_AT_TCP,
        .command_timeout_ms = 15000U,
        .ap_retry_ms = 5000U,
        .server_retry_ms = 3000U,
    };
    const esp_at_io_t io = {
        .write = uart_write,
        .read = uart_read,
        .tick_ms = get_tick,
        .delay_ms = delay_ms,
        .user = &huart1,
    };
    return esp_at_init(&wifi, &config, &io);
}

void App_Loop(void)
{
    esp_at_process(&wifi); /* while (1) 안에서 계속 호출 */
    if (esp_at_is_online(&wifi)) {
        /* 애플리케이션 동작. 매 루프마다 중복 전송하지 않도록 별도 조건 사용. */
    }
}

bool App_Send(const void *data, size_t length)
{
    return esp_at_send(&wifi, data, length);
}
