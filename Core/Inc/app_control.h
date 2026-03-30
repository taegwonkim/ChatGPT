#ifndef APP_CONTROL_H
#define APP_CONTROL_H

#include <stdbool.h>
#include <stdint.h>
#include "app_config.h"

void App_Init(void);
void App_SetVoltage(uint8_t ch, float volt);
void App_GetSnapshot(ch_data_t *out, uint8_t max_count);
void App_ControlStep(void);
void App_TelemetryStep(void);
void App_ParseUartByte(uint8_t rx);

#endif
