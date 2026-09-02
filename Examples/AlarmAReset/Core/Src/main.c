#include "main.h"
#include "rtc.h"
#include "usart.h"

#define LOOP_REPORT_PERIOD_MS  (5000UL)

volatile uint32_t g_reset_cause;
volatile uint32_t g_loop_count;

static void SystemClock_Config(void);
static void RunMode_Configure(void);

int main(void)
{
  uint32_t lastReportTick;

  HAL_Init();
  SystemClock_Config();
  RunMode_Configure();

  g_reset_cause = RCC->CSR;
  __HAL_RCC_CLEAR_RESET_FLAGS();

  MX_USART3_UART_Init();
  if (USART3_SendResetMessage(g_reset_cause) != HAL_OK)
  {
    Error_Handler();
  }

  MX_RTC_Init();
  lastReportTick = HAL_GetTick();

  while (1)
  {
    uint32_t now;

    /* Volatile work keeps the application active in Run mode. */
    g_loop_count++;
    now = HAL_GetTick();

    if ((now - lastReportTick) >= LOOP_REPORT_PERIOD_MS)
    {
      lastReportTick = now;
      if (USART3_SendLoopStatus(g_loop_count, now / 1000UL) != HAL_OK)
      {
        Error_Handler();
      }
    }
  }
}

static void RunMode_Configure(void)
{
  /* Do not automatically enter Sleep after an interrupt and do not select a
   * deep-sleep state. The main loop also contains no WFI/WFE instruction. */
  SCB->SCR &= ~(SCB_SCR_SLEEPONEXIT_Msk | SCB_SCR_SLEEPDEEP_Msk);
  __DSB();
  __ISB();
}

static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);
  HAL_PWR_EnableBkUpAccess();

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI |
                                    RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif
