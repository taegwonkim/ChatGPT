#ifndef APP_DATA_H
#define APP_DATA_H

#include <stdbool.h>
#include <stddef.h>

#include "stm32l5xx_hal.h"

/* USART2로 수신하는 ADC 한 프레임은 '\n'으로 끝난다고 가정합니다. */
bool App_DataInit(void);
void App_DataProcess(void);

/* main.c의 공용 HAL callback에서 호출합니다. */
void App_DataUartRxCpltCallback(UART_HandleTypeDef *huart);
void App_DataUartErrorCallback(UART_HandleTypeDef *huart);

/* 바이너리 ADC 프로토콜을 쓰는 경우 USART2 parser 대신 직접 호출합니다. */
void App_DataSetLatestFromISR(const void *data, size_t length);

#endif
