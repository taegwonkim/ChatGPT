#include "app_control.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "cmsis_os.h"
#include "usart.h"
#include "fdcan.h"
#include "spi.h"

static ch_data_t g_ch[APP_CH_COUNT];
static char g_uart_line[UART_RX_LINE_MAX];
static uint16_t g_uart_idx;

static uint16_t prv_VoltToDacCode(float v)
{
  float clamped = v;
  if (clamped < 0.0f) clamped = 0.0f;
  if (clamped > DAC_OUTPUT_MAX) clamped = DAC_OUTPUT_MAX;

  float norm = clamped / DAC_OUTPUT_MAX;
  uint32_t code = (uint32_t)lrintf(norm * (float)DAC_AD5641_CODE_MAX);
  if (code > DAC_AD5641_CODE_MAX) code = DAC_AD5641_CODE_MAX;
  return (uint16_t)code;
}

static void prv_WriteDacAd5641(uint8_t ch, uint16_t code)
{
  uint8_t tx[3];
  tx[0] = 0x30U | (ch & 0x03U);
  tx[1] = (uint8_t)(code >> 6);
  tx[2] = (uint8_t)(code << 2);
  HAL_SPI_Transmit(&hspi1, tx, sizeof(tx), 10);
}

static float prv_ReadMcp3465rVoltage(uint8_t ch)
{
  (void)ch;
  /* TODO: MCP3465R 레지스터 시퀀스 읽기 구현 */
  return 0.0f;
}

static float prv_ReadMcp3465rCurrent(uint8_t ch)
{
  (void)ch;
  /* TODO: MCP3465R 입력 mux를 shunt 경로로 전환 후 변환값 읽기 */
  return 0.0f;
}

static void prv_UpdateStatus(ch_data_t *d)
{
  if (d->i_meas >= CH_SHORT_CURRENT_A)
  {
    d->status = CH_STATUS_SHORT;
  }
  else if (d->i_meas <= CH_OPEN_CURRENT_A)
  {
    d->status = CH_STATUS_OPEN;
  }
  else if (d->dac_code >= DAC_AD5641_CODE_MAX)
  {
    d->status = CH_STATUS_SAT_HIGH;
  }
  else if (d->dac_code == 0U)
  {
    d->status = CH_STATUS_SAT_LOW;
  }
  else
  {
    d->status = CH_STATUS_NORMAL;
  }
}

static void prv_ClosedLoopStep(ch_data_t *d)
{
  const float kp = 0.25f;
  d->v_err = d->v_set - d->v_meas;
  if (fabsf(d->v_err) > CH_VOLTAGE_TOLERANCE_V)
  {
    float corrected = d->v_set + kp * d->v_err;
    d->dac_code = prv_VoltToDacCode(corrected);
  }
  else
  {
    d->dac_code = prv_VoltToDacCode(d->v_set);
  }
}

void App_Init(void)
{
  memset(g_ch, 0, sizeof(g_ch));
  g_uart_idx = 0;

  for (uint8_t i = 0; i < APP_CH_COUNT; i++)
  {
    g_ch[i].v_set = 1.0f;
    g_ch[i].dac_code = prv_VoltToDacCode(g_ch[i].v_set);
    prv_WriteDacAd5641(i, g_ch[i].dac_code);
  }
}

void App_SetVoltage(uint8_t ch, float volt)
{
  if (ch >= APP_CH_COUNT) return;
  g_ch[ch].v_set = volt;
}

void App_GetSnapshot(ch_data_t *out, uint8_t max_count)
{
  if ((out == NULL) || (max_count == 0U)) return;
  uint8_t count = (max_count > APP_CH_COUNT) ? APP_CH_COUNT : max_count;
  memcpy(out, g_ch, sizeof(ch_data_t) * count);
}

void App_ControlStep(void)
{
  for (uint8_t i = 0; i < APP_CH_COUNT; i++)
  {
    g_ch[i].v_meas = prv_ReadMcp3465rVoltage(i) * ADC_VOLTAGE_SCALE;
    g_ch[i].i_meas = prv_ReadMcp3465rCurrent(i) * ADC_CURRENT_SCALE;

    prv_ClosedLoopStep(&g_ch[i]);
    prv_WriteDacAd5641(i, g_ch[i].dac_code);
    prv_UpdateStatus(&g_ch[i]);
  }
}

void App_TelemetryStep(void)
{
  char msg[256];
  int len = snprintf(msg, sizeof(msg),
                     "CH0:%.3f/%.3f/%.3f/%u," \
                     "CH1:%.3f/%.3f/%.3f/%u," \
                     "CH2:%.3f/%.3f/%.3f/%u," \
                     "CH3:%.3f/%.3f/%.3f/%u\r\n",
                     g_ch[0].v_set, g_ch[0].v_meas, g_ch[0].i_meas, g_ch[0].status,
                     g_ch[1].v_set, g_ch[1].v_meas, g_ch[1].i_meas, g_ch[1].status,
                     g_ch[2].v_set, g_ch[2].v_meas, g_ch[2].i_meas, g_ch[2].status,
                     g_ch[3].v_set, g_ch[3].v_meas, g_ch[3].i_meas, g_ch[3].status);

  if (len > 0)
  {
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)len, 20);
  }

  FDCAN_TxHeaderTypeDef txh = {0};
  uint8_t payload[64] = {0};

  txh.Identifier = 0x321;
  txh.IdType = FDCAN_STANDARD_ID;
  txh.TxFrameType = FDCAN_DATA_FRAME;
  txh.DataLength = FDCAN_DLC_BYTES_64;
  txh.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  txh.BitRateSwitch = FDCAN_BRS_ON;
  txh.FDFormat = FDCAN_FD_CAN;
  txh.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  txh.MessageMarker = 0;

  memcpy(&payload[0], g_ch, sizeof(g_ch));
  HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &txh, payload);
}

static void prv_HandleCommand(const char *line)
{
  uint8_t ch;
  float v;
  if (sscanf(line, "SET CH%hhu %f", &ch, &v) == 2)
  {
    App_SetVoltage(ch, v);
  }
}

void App_ParseUartByte(uint8_t rx)
{
  if ((rx == '\r') || (rx == '\n'))
  {
    g_uart_line[g_uart_idx] = '\0';
    prv_HandleCommand(g_uart_line);
    g_uart_idx = 0;
    return;
  }

  if (g_uart_idx < (UART_RX_LINE_MAX - 1U))
  {
    g_uart_line[g_uart_idx++] = (char)rx;
  }
  else
  {
    g_uart_idx = 0;
  }
}
