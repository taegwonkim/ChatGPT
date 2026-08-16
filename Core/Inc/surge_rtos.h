#ifndef SURGE_RTOS_H
#define SURGE_RTOS_H

#include "cmsis_os2.h"

/* Handles are exported only for HAL/USB callbacks that must notify a task. */
extern osThreadId_t fpgaRxTaskHandle;
extern osMessageQueueId_t pcCommandQueueHandle;

/* Initializes application state and starts UART receive-to-idle DMA. */
void Surge_AppHardwareStart(void);

/* Task entry functions implemented by the application layer. */
void Surge_FpgaRxTask(void *argument);
void Surge_DataRouterTask(void *argument);
void Surge_WifiTask(void *argument);
void Surge_PcCommandTask(void *argument);
void Surge_FlashTask(void *argument);
void Surge_SupervisorTask(void *argument);

#endif
