#ifndef TEST_MAIN_H
#define TEST_MAIN_H

#include <stdint.h>

typedef struct {
    uint32_t instance;
} UART_HandleTypeDef;

typedef enum { HAL_OK = 0, HAL_ERROR = 1 } HAL_StatusTypeDef;

uint32_t HAL_GetTick(void);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *uart, uint8_t *data,
                                    uint16_t length, uint32_t timeout);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *uart, uint8_t *data,
                                     uint16_t length);

#endif
