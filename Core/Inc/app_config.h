#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

#define APP_CH_COUNT                4U
#define APP_CONTROL_PERIOD_MS       10U
#define APP_TELEMETRY_PERIOD_MS     100U
#define APP_WATCHDOG_PERIOD_MS      50U

#define DAC_AD5641_BITS             14U
#define DAC_AD5641_CODE_MAX         ((1U << DAC_AD5641_BITS) - 1U)
#define DAC_REF_VOLTAGE             2.500f
#define DAC_GAIN                    2.0f
#define DAC_OUTPUT_MAX              (DAC_REF_VOLTAGE * DAC_GAIN)

#define ADC_VOLTAGE_SCALE           1.000f
#define ADC_CURRENT_SCALE           1.000f

#define CH_SHORT_CURRENT_A          0.150f
#define CH_OPEN_CURRENT_A           0.002f
#define CH_VOLTAGE_TOLERANCE_V      0.010f

#define UART_RX_LINE_MAX            96U

typedef enum
{
  CH_STATUS_NORMAL = 0,
  CH_STATUS_OPEN,
  CH_STATUS_SHORT,
  CH_STATUS_SAT_HIGH,
  CH_STATUS_SAT_LOW
} ch_status_t;

typedef struct
{
  float v_set;
  float v_meas;
  float i_meas;
  float v_err;
  uint16_t dac_code;
  ch_status_t status;
} ch_data_t;

#endif
