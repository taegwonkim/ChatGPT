/* STM32CubeIDE/CubeMX가 생성한 USART1을 사용하는 ESP-AT 어댑터입니다. */
#include "app_wifi.h"

#include "esp32c3_at.h"
#include "app_data.h"
#include "usart.h" /* CubeMX 생성 파일: extern UART_HandleTypeDef huart1 */

#define WIFI_RX_RING_SIZE 2048U /* 반드시 2의 거듭제곱 */

static esp_at_t wifi;
static uint8_t uart_rx_byte;
static uint8_t rx_ring[WIFI_RX_RING_SIZE];
static volatile uint16_t rx_write_index;
static volatile uint16_t rx_read_index;

static void start_uart_receive(void)
{
    (void)HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1U);
}

void App_WifiUartRxCpltCallback(UART_HandleTypeDef *huart)
{
    uint16_t next;

    if (huart != &huart1) {
        return;
    }

    next = (uint16_t)((rx_write_index + 1U) & (WIFI_RX_RING_SIZE - 1U));
    if (next != rx_read_index) {
        rx_ring[rx_write_index] = uart_rx_byte;
        rx_write_index = next;
    }
    start_uart_receive();
}

void App_WifiUartErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1) {
        /* Overrun 등의 오류 뒤 HAL 상태를 복구하고 1-byte IT 수신을 재개합니다. */
        (void)HAL_UART_AbortReceive(huart);
        start_uart_receive();
    }
}

static size_t uart_write(const uint8_t *data, size_t length, void *user)
{
    UART_HandleTypeDef *uart = user;

    if (length > UINT16_MAX) {
        return 0U;
    }
    return HAL_UART_Transmit(uart, (uint8_t *)data, (uint16_t)length, 1000U) ==
                   HAL_OK
               ? length
               : 0U;
}

static size_t uart_read(uint8_t *data, size_t capacity, uint32_t timeout_ms,
                        void *user)
{
    uint32_t start = HAL_GetTick();
    size_t count = 0U;
    (void)user;

    do {
        while (count < capacity && rx_read_index != rx_write_index) {
            data[count++] = rx_ring[rx_read_index];
            rx_read_index = (uint16_t)((rx_read_index + 1U) &
                                       (WIFI_RX_RING_SIZE - 1U));
        }
        if (count != 0U || timeout_ms == 0U) {
            break;
        }
    } while ((uint32_t)(HAL_GetTick() - start) < timeout_ms);

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

static void background_process(void *user)
{
    (void)user;
    /* AP/server 재접속 명령을 기다리는 동안에도 ADC/PC 주기를 처리합니다. */
    App_DataProcess();
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
        .background_process = background_process,
        .user = &huart1,
    };

    rx_write_index = 0U;
    rx_read_index = 0U;
    start_uart_receive();
    return esp_at_init(&wifi, &config, &io);
}

void App_WifiProcess(void)
{
    esp_at_process(&wifi);
}

bool App_WifiIsOnline(void)
{
    return esp_at_is_online(&wifi);
}

bool App_WifiSend(const void *data, size_t length)
{
    return esp_at_send(&wifi, data, length);
}
