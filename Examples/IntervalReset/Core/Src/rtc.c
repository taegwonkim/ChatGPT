#include "rtc.h"
#include "main.h"

RTC_HandleTypeDef hrtc;

/* ck_spre provides one tick per second. The STM32L562 17-bit wake-up mode is
 * required because a 24-hour interval does not fit in the 16-bit counter.
 * The timer expires after WakeUpCounter + 1 ticks. */
#define RTC_RESET_PERIOD_SECONDS  (24UL * 60UL * 60UL)
#define RTC_WAKEUP_COUNTER        (RTC_RESET_PERIOD_SECONDS - 1UL)

#if RTC_WAKEUP_COUNTER > 0x1FFFFUL
#error "RTC reset period exceeds the 17-bit wake-up counter range"
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

  /* The RTC backup domain survives a software reset. Remove any timer state
   * left by the previous run, then start a fresh 24-hour interval. */
  if (HAL_RTCEx_DeactivateWakeUpTimer(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* Enable the NVIC only after the retained wake-up flag has been cleared.
   * Enabling it in HAL_RTC_MspInit() could service a stale flag immediately
   * after a software reset and create a reset loop. */
  HAL_NVIC_ClearPendingIRQ(RTC_IRQn);
  HAL_NVIC_SetPriority(RTC_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(RTC_IRQn);

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
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef *rtcHandle)
{
  if (rtcHandle->Instance == RTC)
  {
    __HAL_RCC_RTCAPB_CLK_DISABLE();
    HAL_NVIC_DisableIRQ(RTC_IRQn);
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
