#include "cmsis_os.h"
#include "app_control.h"
#include "usart.h"

osThreadId_t controlTaskHandle;
osThreadId_t telemetryTaskHandle;
osThreadId_t uartRxTaskHandle;

osMessageQueueId_t uartRxQueueHandle;
osMutexId_t appDataMutexHandle;

static uint8_t g_uartRxByte;

static void StartControlTask(void *argument);
static void StartTelemetryTask(void *argument);
static void StartUartRxTask(void *argument);

void MX_FREERTOS_Init(void)
{
  const osMessageQueueAttr_t uartRxQueue_attributes = {
    .name = "uartRxQueue"
  };
  uartRxQueueHandle = osMessageQueueNew(128, sizeof(uint8_t), &uartRxQueue_attributes);

  const osMutexAttr_t appDataMutex_attributes = {
    .name = "appDataMutex"
  };
  appDataMutexHandle = osMutexNew(&appDataMutex_attributes);

  const osThreadAttr_t controlTask_attributes = {
    .name = "controlTask",
    .stack_size = 1024,
    .priority = (osPriority_t) osPriorityAboveNormal
  };
  controlTaskHandle = osThreadNew(StartControlTask, NULL, &controlTask_attributes);

  const osThreadAttr_t telemetryTask_attributes = {
    .name = "telemetryTask",
    .stack_size = 1024,
    .priority = (osPriority_t) osPriorityNormal
  };
  telemetryTaskHandle = osThreadNew(StartTelemetryTask, NULL, &telemetryTask_attributes);

  const osThreadAttr_t uartRxTask_attributes = {
    .name = "uartRxTask",
    .stack_size = 768,
    .priority = (osPriority_t) osPriorityHigh
  };
  uartRxTaskHandle = osThreadNew(StartUartRxTask, NULL, &uartRxTask_attributes);
}

static void StartControlTask(void *argument)
{
  (void)argument;
  App_Init();

  TickType_t wake = xTaskGetTickCount();
  for (;;)
  {
    osMutexAcquire(appDataMutexHandle, osWaitForever);
    App_ControlStep();
    osMutexRelease(appDataMutexHandle);

    vTaskDelayUntil(&wake, pdMS_TO_TICKS(APP_CONTROL_PERIOD_MS));
  }
}

static void StartTelemetryTask(void *argument)
{
  (void)argument;

  TickType_t wake = xTaskGetTickCount();
  for (;;)
  {
    osMutexAcquire(appDataMutexHandle, osWaitForever);
    App_TelemetryStep();
    osMutexRelease(appDataMutexHandle);

    vTaskDelayUntil(&wake, pdMS_TO_TICKS(APP_TELEMETRY_PERIOD_MS));
  }
}

static void StartUartRxTask(void *argument)
{
  (void)argument;
  HAL_UART_Receive_IT(&huart1, &g_uartRxByte, 1);

  uint8_t ch;
  for (;;)
  {
    if (osMessageQueueGet(uartRxQueueHandle, &ch, NULL, osWaitForever) == osOK)
    {
      osMutexAcquire(appDataMutexHandle, osWaitForever);
      App_ParseUartByte(ch);
      osMutexRelease(appDataMutexHandle);
    }
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    osMessageQueuePut(uartRxQueueHandle, &g_uartRxByte, 0U, 0U);
    HAL_UART_Receive_IT(&huart1, &g_uartRxByte, 1);
  }
}
