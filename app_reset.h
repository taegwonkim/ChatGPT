#ifndef APP_RESET_H
#define APP_RESET_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32l5xx_hal.h"

/*
 * RTC wake-up timer 기반 주기 software reset 및 USART3 설정 명령 처리.
 * period_seconds == 0이면 자동 reset을 사용하지 않습니다.
 */
bool App_ResetInit(void);
void App_ResetProcess(void);
uint32_t App_ResetGetPeriod(void);
bool App_ResetSetPeriod(uint32_t period_seconds);

void App_ResetUartRxCpltCallback(UART_HandleTypeDef *huart);
void App_ResetUartErrorCallback(UART_HandleTypeDef *huart);
void App_ResetRtcWakeupCallback(RTC_HandleTypeDef *hrtc_handle);

#endif
