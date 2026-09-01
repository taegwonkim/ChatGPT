#include "rtc.h"
#include "main.h"

RTC_HandleTypeDef hrtc;

/* The 17-bit ck_spre mode is required because 24 hours does not fit in the
 * 16-bit wake-up counter. One count is one second and the hardware expires
 * after WakeUpCounter + 1 ticks, therefore 86399 means 86400 seconds. */
#define RTC_RESET_PERIOD_SECONDS  (24UL * 60UL * 60UL)
#define RTC_WAKEUP_COUNTER        (RTC_RESET_PERIOD_SECONDS - 1UL)

#if RTC_WAKEUP_COUNTER > 0x1FFFFUL
#error "RTC wake-up period exceeds the 17-bit counter range"
#endif

void MX_RTC_Init(void)
{
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* CubeMX/HAL may leave a running timer in the backup domain after a reset. */
  if (HAL_RTCEx_DeactivateWakeUpTimer(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, RTC_WAKEUP_COUNTER,
                                  RTC_WAKEUPCLOCK_CK_SPRE_17BITS) != HAL_OK)
  {
    Error_Handler();
  }
}

void HAL_RTC_MspInit(RTC_HandleTypeDef *rtcHandle)
{
  if (rtcHandle->Instance == RTC)
  {
    __HAL_RCC_RTCAPB_CLK_ENABLE();
    HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef *rtcHandle)
{
  if (rtcHandle->Instance == RTC)
  {
    __HAL_RCC_RTCAPB_CLK_DISABLE();
    HAL_NVIC_DisableIRQ(RTC_WKUP_IRQn);
  }
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *rtcHandle)
{
  if (rtcHandle->Instance == RTC)
  {
    __DSB();
    NVIC_SystemReset();
  }
}
