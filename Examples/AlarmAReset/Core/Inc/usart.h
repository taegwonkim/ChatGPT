#ifndef __USART_H
#define __USART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l5xx_hal.h"

extern UART_HandleTypeDef huart3;

void MX_USART3_UART_Init(void);
HAL_StatusTypeDef USART3_SendResetMessage(uint32_t resetFlags);
HAL_StatusTypeDef USART3_SendLoopStatus(uint32_t loopCount,
                                        uint32_t uptimeSeconds);

#ifdef __cplusplus
}
#endif

#endif /* __USART_H */

