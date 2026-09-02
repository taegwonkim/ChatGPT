#include "usart.h"
#include "main.h"

#include <stdio.h>

UART_HandleTypeDef huart3;

#define USART3_TX_TIMEOUT_MS  (100U)

void MX_USART3_UART_Init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
}

HAL_StatusTypeDef USART3_SendResetMessage(uint32_t resetFlags)
{
  char message[96];
  const char *resetType = ((resetFlags & RCC_CSR_SFTRSTF) != 0U) ?
                          "SOFTWARE" : "POWER/OTHER";
  int length = snprintf(message, sizeof(message),
                        "\r\n[BOOT] MCU reset: type=%s, RCC_CSR=0x%08lX\r\n",
                        resetType, (unsigned long)resetFlags);

  if ((length < 0) || ((size_t)length >= sizeof(message)))
  {
    return HAL_ERROR;
  }

  return HAL_UART_Transmit(&huart3, (uint8_t *)message, (uint16_t)length,
                           USART3_TX_TIMEOUT_MS);
}

HAL_StatusTypeDef USART3_SendLoopStatus(uint32_t loopCount,
                                        uint32_t uptimeSeconds)
{
  char message[80];
  int length = snprintf(message, sizeof(message),
                        "[LOOP] count=%lu, uptime=%lu s\r\n",
                        (unsigned long)loopCount,
                        (unsigned long)uptimeSeconds);

  if ((length < 0) || ((size_t)length >= sizeof(message)))
  {
    return HAL_ERROR;
  }

  return HAL_UART_Transmit(&huart3, (uint8_t *)message, (uint16_t)length,
                           USART3_TX_TIMEOUT_MS);
}

void HAL_UART_MspInit(UART_HandleTypeDef *uartHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (uartHandle->Instance == USART3)
  {
    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PB10 = USART3_TX, PB11 = USART3_RX. */
    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *uartHandle)
{
  if (uartHandle->Instance == USART3)
  {
    __HAL_RCC_USART3_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10 | GPIO_PIN_11);
  }
}

