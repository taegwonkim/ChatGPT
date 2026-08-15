#pragma once
#include "stm32l5xx_hal.h"
#include <stdint.h>

void App_Init(UART_HandleTypeDef *esp, UART_HandleTypeDef *fpga,
              UART_HandleTypeDef *pc, SPI_HandleTypeDef *flash);
void App_RTOS_Init(void);
void App_UartRxEventFromISR(UART_HandleTypeDef *huart, uint16_t size);
void App_UsbRx(const uint8_t *data, uint32_t length);
