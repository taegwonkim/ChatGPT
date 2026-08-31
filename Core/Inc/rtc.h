#ifndef __RTC_H
#define __RTC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l5xx_hal.h"

extern RTC_HandleTypeDef hrtc;

void MX_RTC_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __RTC_H */

