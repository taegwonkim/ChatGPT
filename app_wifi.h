#ifndef APP_WIFI_H
#define APP_WIFI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "stm32l5xx_hal.h"

bool App_WifiInit(void);
void App_WifiProcess(void);
bool App_WifiIsOnline(void);
bool App_WifiSend(const void *data, size_t length);

/* main.c의 HAL callback에서 호출해야 하는 USART1 수신 전달 함수입니다. */
void App_WifiUartRxCpltCallback(UART_HandleTypeDef *huart);
void App_WifiUartErrorCallback(UART_HandleTypeDef *huart);

#endif
