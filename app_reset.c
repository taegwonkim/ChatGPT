#include "app_reset.h"

#include "rtc.h"   /* CubeMX 생성: hrtc */
#include "usart.h" /* CubeMX 생성: huart3 */

#include <stdio.h>
#include <string.h>

#define RESET_PERIOD_MAX_SECONDS 31536000UL /* 365 days */
#define RESET_WAKEUP_CHUNK_MAX    65536UL
#define RESET_BACKUP_MAGIC        0x52535431UL /* "RST1" */
#define RESET_RX_MAX              64U
#define RESET_TX_TIMEOUT_MS       100U

static uint8_t rx_byte;
static char rx_build[RESET_RX_MAX];
static volatile uint16_t rx_length;
static char command[RESET_RX_MAX];
static volatile bool command_ready;
static bool receiving_frame;
static bool saw_cr;
static uint32_t reset_period_seconds;
static uint32_t remaining_seconds;

static void start_pc_receive(void)
{
    (void)HAL_UART_Receive_IT(&huart3, &rx_byte, 1U);
}

static bool arm_next_chunk(void)
{
    uint32_t chunk;

    if (remaining_seconds == 0U) {
        return true;
    }
    chunk = remaining_seconds > RESET_WAKEUP_CHUNK_MAX
        ? RESET_WAKEUP_CHUNK_MAX : remaining_seconds;
    remaining_seconds -= chunk;
    (void)HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
    return HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, chunk - 1U,
        RTC_WAKEUPCLOCK_CK_SPRE_16BITS) == HAL_OK;
}

bool App_ResetSetPeriod(uint32_t period_seconds)
{
    if (period_seconds > RESET_PERIOD_MAX_SECONDS) {
        return false;
    }

    (void)HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
    reset_period_seconds = period_seconds;
    remaining_seconds = period_seconds;
    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, RESET_BACKUP_MAGIC);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, period_seconds);
    return period_seconds == 0U || arm_next_chunk();
}

uint32_t App_ResetGetPeriod(void)
{
    return reset_period_seconds;
}

bool App_ResetInit(void)
{
    uint32_t saved_period = 0U;

    rx_length = 0U;
    command_ready = false;
    receiving_frame = false;
    saw_cr = false;
    HAL_PWR_EnableBkUpAccess();
    if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) == RESET_BACKUP_MAGIC) {
        saved_period = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
        if (saved_period > RESET_PERIOD_MAX_SECONDS) {
            saved_period = 0U;
        }
    }
    start_pc_receive();
    return App_ResetSetPeriod(saved_period);
}

static void send_period_response(void)
{
    char response[40];
    int length = snprintf(response, sizeof(response), "\x02RESET_R_ALL,%lu\r\n",
        (unsigned long)reset_period_seconds);
    if (length > 0 && (size_t)length < sizeof(response)) {
        (void)HAL_UART_Transmit(&huart3, (uint8_t *)response,
            (uint16_t)length, RESET_TX_TIMEOUT_MS);
    }
}

void App_ResetProcess(void)
{
    char local[RESET_RX_MAX];
    char trailing;
    unsigned long seconds;

    if (!command_ready) {
        return;
    }
    __disable_irq();
    memcpy(local, command, sizeof(local));
    command_ready = false;
    __enable_irq();

    if (strcmp(local, "RESET_R_ALL") == 0) {
        send_period_response();
    } else if (sscanf(local, "RESET_W_ALL,%lu%c", &seconds, &trailing) == 1 &&
               seconds <= RESET_PERIOD_MAX_SECONDS &&
               App_ResetSetPeriod((uint32_t)seconds)) {
        send_period_response();
    }
}

void App_ResetUartRxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != &huart3) {
        return;
    }
    if (rx_byte == 0x02U) {
        receiving_frame = true;
        saw_cr = false;
        rx_length = 0U;
    } else if (receiving_frame && saw_cr) {
        if (rx_byte == '\n' && rx_length < sizeof(rx_build)) {
            rx_build[rx_length] = '\0';
            if (!command_ready) {
                memcpy(command, rx_build, rx_length + 1U);
                command_ready = true;
            }
        }
        receiving_frame = false;
        saw_cr = false;
    } else if (receiving_frame && rx_byte == '\r') {
        saw_cr = true;
    } else if (receiving_frame && rx_length < sizeof(rx_build) - 1U) {
        rx_build[rx_length++] = (char)rx_byte;
    } else if (receiving_frame) {
        receiving_frame = false;
        rx_length = 0U;
    }
    start_pc_receive();
}

void App_ResetUartErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart3) {
        (void)HAL_UART_AbortReceive(huart);
        receiving_frame = false;
        rx_length = 0U;
        start_pc_receive();
    }
}

void App_ResetRtcWakeupCallback(RTC_HandleTypeDef *hrtc_handle)
{
    if (hrtc_handle != &hrtc || reset_period_seconds == 0U) {
        return;
    }
    if (remaining_seconds != 0U) {
        if (!arm_next_chunk()) {
            NVIC_SystemReset();
        }
        return;
    }
    NVIC_SystemReset();
}
