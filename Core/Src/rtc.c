#include "rtc.h"
#include "main.h"

RTC_HandleTypeDef hrtc;

/* Alarm A is configured as a daily alarm. Change these values if the reset
 * must occur at another time of day. */
#define RTC_ALARM_HOUR            (0U)
#define RTC_ALARM_MINUTE          (0U)
#define RTC_ALARM_SECOND          (0U)
#define RTC_CALENDAR_MAGIC        (0x52544341UL)

static void RTC_InitializeCalendarIfNeeded(void);
static void RTC_WaitUntilAlarmSecondHasPassed(void);

void MX_RTC_Init(void)
{
  RTC_AlarmTypeDef alarm = {0};

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

  RTC_InitializeCalendarIfNeeded();

  /* Alarm A is retained in the backup domain across a software reset. Always
   * remove the previous configuration before installing the daily alarm. */
  if (HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A) != HAL_OK)
  {
    Error_Handler();
  }

  /* If the reset/reboot completes during 00:00:00, enabling a daily alarm at
   * once could match again and cause a reset loop. Wait for that second to
   * finish before re-arming Alarm A. */
  RTC_WaitUntilAlarmSecondHasPassed();

  alarm.AlarmTime.Hours = RTC_ALARM_HOUR;
  alarm.AlarmTime.Minutes = RTC_ALARM_MINUTE;
  alarm.AlarmTime.Seconds = RTC_ALARM_SECOND;
  alarm.AlarmTime.SubSeconds = 0U;
  alarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  alarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
  alarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;
  alarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
  alarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  alarm.AlarmDateWeekDay = 1U; /* Ignored because DATEWEEKDAY is masked. */
  alarm.Alarm = RTC_ALARM_A;

  if (HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
}

static void RTC_InitializeCalendarIfNeeded(void)
{
  RTC_TimeTypeDef time = {0};
  RTC_DateTypeDef date = {0};

  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) == RTC_CALENDAR_MAGIC)
  {
    return;
  }

  /* A product with an external time source should replace these defaults with
   * its actual local/UTC date and time before enabling the daily alarm. Start
   * at 00:00:01 so the initial boot cannot immediately match Alarm A. */
  time.Hours = 0U;
  time.Minutes = 0U;
  time.Seconds = 1U;
  time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  time.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }

  date.WeekDay = RTC_WEEKDAY_MONDAY;
  date.Month = RTC_MONTH_JANUARY;
  date.Date = 1U;
  date.Year = 24U;
  if (HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, RTC_CALENDAR_MAGIC);
}

static void RTC_WaitUntilAlarmSecondHasPassed(void)
{
  RTC_TimeTypeDef time = {0};
  RTC_DateTypeDef date = {0};
  uint32_t startedAt = HAL_GetTick();

  do
  {
    if ((HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK) ||
        (HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK))
    {
      Error_Handler();
    }

    if ((time.Hours != RTC_ALARM_HOUR) ||
        (time.Minutes != RTC_ALARM_MINUTE) ||
        (time.Seconds != RTC_ALARM_SECOND))
    {
      return;
    }
  } while ((HAL_GetTick() - startedAt) < 2000U);

  /* A running 1 Hz calendar must leave the alarm second within one second. */
  Error_Handler();
}

void HAL_RTC_MspInit(RTC_HandleTypeDef *rtcHandle)
{
  if (rtcHandle->Instance == RTC)
  {
    __HAL_RCC_RTCAPB_CLK_ENABLE();
    HAL_NVIC_SetPriority(RTC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_IRQn);
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

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *rtcHandle)
{
  if (rtcHandle->Instance == RTC)
  {
    __DSB();
    NVIC_SystemReset();
  }
}
