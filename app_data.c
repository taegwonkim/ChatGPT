/* USART2 ADC 입력을 1초마다 Wi-Fi 서버 및 USART3 PC로 전달합니다. */
#include "app_data.h"

#include "app_wifi.h"
#include "usart.h" /* CubeMX 생성: huart2(ADC), huart3(PC) */

#include <string.h>

#define ADC_FRAME_MAX       128U
#define DATA_PERIOD_MS      1000U
#define PC_TX_TIMEOUT_MS    100U

static uint8_t adc_rx_byte;
static uint8_t building_frame[ADC_FRAME_MAX];
static volatile size_t building_length;
static uint8_t latest_frame[ADC_FRAME_MAX];
static volatile size_t latest_length;
static uint32_t next_send_ms;
static bool initialized;
static bool server_sending;

static void start_adc_receive(void)
{
    (void)HAL_UART_Receive_IT(&huart2, &adc_rx_byte, 1U);
}

void App_DataSetLatestFromISR(const void *data, size_t length)
{
    if (data == NULL || length == 0U) {
        return;
    }
    if (length > sizeof(latest_frame)) {
        length = sizeof(latest_frame);
    }
    memcpy(latest_frame, data, length);
    latest_length = length;
}

void App_DataUartRxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != &huart2) {
        return;
    }

    if (building_length < sizeof(building_frame)) {
        building_frame[building_length++] = adc_rx_byte;
    }

    if (adc_rx_byte == '\n') {
        App_DataSetLatestFromISR(building_frame, building_length);
        building_length = 0U;
    } else if (building_length == sizeof(building_frame)) {
        /* 잘린 프레임을 보내지 않고 다음 newline부터 다시 동기화합니다. */
        building_length = 0U;
    }
    start_adc_receive();
}

void App_DataUartErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart2) {
        (void)HAL_UART_AbortReceive(huart);
        building_length = 0U;
        start_adc_receive();
    }
}

bool App_DataInit(void)
{
    building_length = 0U;
    latest_length = 0U;
    next_send_ms = HAL_GetTick() + DATA_PERIOD_MS;
    initialized = true;
    server_sending = false;
    start_adc_receive();
    return true;
}

void App_DataProcess(void)
{
    uint8_t snapshot[ADC_FRAME_MAX];
    size_t length;
    uint32_t now;

    if (!initialized) {
        return;
    }
    now = HAL_GetTick();
    if ((int32_t)(now - next_send_ms) < 0) {
        return;
    }
    next_send_ms += DATA_PERIOD_MS;
    if ((int32_t)(now - next_send_ms) >= 0) {
        next_send_ms = now + DATA_PERIOD_MS;
    }

    /* ISR 갱신과 일관된 snapshot을 만들기 위한 매우 짧은 임계 구역입니다. */
    __disable_irq();
    length = latest_length;
    if (length != 0U) {
        memcpy(snapshot, latest_frame, length);
    }
    __enable_irq();

    if (length != 0U) {
        /* PC 출력은 Wi-Fi 상태와 관계없이 매 1초마다 수행합니다. */
        (void)HAL_UART_Transmit(&huart3, snapshot, (uint16_t)length,
                                PC_TX_TIMEOUT_MS);

        /* 서버는 online일 때만 송신합니다. 실패하면 Wi-Fi 모듈이 재접속합니다. */
        if (!server_sending && App_WifiIsOnline()) {
            server_sending = true;
            (void)App_WifiSend(snapshot, length);
            server_sending = false;
        }
    }
}
