/*
 * STM32L552 + MCP3465R(ADC, SPI) + DAC80501(DAC, SPI)
 * 자동 캘리브레이션 예시 코드
 *
 * 요구사항:
 *  - DAC 출력 0.5V 인가 시 offset 보정점 측정
 *  - DAC 출력 4.5V 인가 시 gain 보정점 측정
 *  - MCU 내부에서 자동 계산 후 실시간 보정값 제공
 *
 * 주의:
 *  - 아래 레지스터 주소/비트 정의는 프로젝트에서 사용하는 데이터시트 값으로 반드시 검증하세요.
 *  - SPI 모드, 최대 클럭, CS 타이밍, LDAC 핀 동작은 보드 하드웨어에 맞게 조정하세요.
 */

#include "stm32l5xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* ========================= 사용자 설정 영역 ========================= */
extern SPI_HandleTypeDef hspi1; /* MCP3465R용 SPI */
extern SPI_HandleTypeDef hspi2; /* DAC80501용 SPI */

#define MCP3465_SPI_HANDLE        hspi1
#define DAC80501_SPI_HANDLE       hspi2

/* CS 핀 (예시) */
#define MCP3465_CS_GPIO_Port      GPIOA
#define MCP3465_CS_Pin            GPIO_PIN_4

#define DAC80501_CS_GPIO_Port     GPIOB
#define DAC80501_CS_Pin           GPIO_PIN_12

/* ADC 기준전압 및 해상도 설정(프로젝트 실제값으로 교체) */
#define MCP3465_VREF              2.500f
#define MCP3465_PGA_GAIN          1.0f
#define MCP3465_FULL_SCALE_CODE   8388607.0f   /* 24-bit signed max */

/* DAC 설정 */
#define DAC80501_VREF             5.000f       /* DAC 출력 full-scale 기준 전압 */
#define DAC80501_MAX_CODE         65535u       /* 16-bit DAC */

/* 캘리브레이션 타깃 전압 */
#define CAL_VOLT_OFFSET_POINT     0.5f
#define CAL_VOLT_GAIN_POINT       4.5f

/* 정착 시간 및 샘플링 설정 */
#define DAC_SETTLE_DELAY_MS       20u
#define ADC_SAMPLES_PER_POINT     64u

/* 품질 체크 임계값 */
#define MIN_DELTA_VOLT            0.100f       /* 두 포인트 차이가 너무 작으면 실패 */

/* ========================= MCP3465R 레지스터 (예시) ========================= */
/* 프로젝트에서 반드시 데이터시트 기준으로 재확인 */
#define MCP3465_CMD_STATIC_WRITE      0x40u
#define MCP3465_CMD_STATIC_READ       0x41u
#define MCP3465_REG_CONFIG0           0x01u
#define MCP3465_REG_CONFIG1           0x02u
#define MCP3465_REG_CONFIG2           0x03u
#define MCP3465_REG_CONFIG3           0x04u
#define MCP3465_REG_MUX               0x06u

/* ========================= DAC80501 레지스터 (예시) ========================= */
#define DAC80501_REG_NOOP             0x00u
#define DAC80501_REG_DEVID            0x01u
#define DAC80501_REG_SYNC             0x02u
#define DAC80501_REG_CONFIG           0x03u
#define DAC80501_REG_GAIN             0x04u
#define DAC80501_REG_TRIGGER          0x05u
#define DAC80501_REG_STATUS           0x07u
#define DAC80501_REG_DAC              0x08u

/* ========================= 데이터 구조 ========================= */
typedef struct {
    float a_gain;       /* y = a*x + b 의 a */
    float b_offset;     /* y = a*x + b 의 b */
    float raw_at_0p5v;  /* 진단용 저장 */
    float raw_at_4p5v;  /* 진단용 저장 */
    bool  valid;
} calibration_t;

static calibration_t g_cal = {
    .a_gain = 1.0f,
    .b_offset = 0.0f,
    .raw_at_0p5v = 0.0f,
    .raw_at_4p5v = 0.0f,
    .valid = false,
};

/* ========================= 저수준 유틸 ========================= */
static inline void cs_low(GPIO_TypeDef *port, uint16_t pin)
{
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
}

static inline void cs_high(GPIO_TypeDef *port, uint16_t pin)
{
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
}

static HAL_StatusTypeDef spi_txrx(SPI_HandleTypeDef *hspi,
                                  uint8_t *tx, uint8_t *rx, uint16_t len)
{
    return HAL_SPI_TransmitReceive(hspi, tx, rx, len, HAL_MAX_DELAY);
}

/* ========================= DAC80501 함수 ========================= */
static HAL_StatusTypeDef DAC80501_WriteReg(uint8_t reg, uint16_t data)
{
    /* DAC80501: 24-bit frame [7:0 addr/cmd][15:0 data] 형태 예시 */
    uint8_t tx[3];
    uint8_t rx[3] = {0};

    tx[0] = reg;
    tx[1] = (uint8_t)((data >> 8) & 0xFFu);
    tx[2] = (uint8_t)(data & 0xFFu);

    cs_low(DAC80501_CS_GPIO_Port, DAC80501_CS_Pin);
    HAL_StatusTypeDef st = spi_txrx(&DAC80501_SPI_HANDLE, tx, rx, 3);
    cs_high(DAC80501_CS_GPIO_Port, DAC80501_CS_Pin);

    return st;
}

static uint16_t DAC80501_VoltageToCode(float vout)
{
    if (vout < 0.0f) vout = 0.0f;
    if (vout > DAC80501_VREF) vout = DAC80501_VREF;

    float code_f = (vout / DAC80501_VREF) * (float)DAC80501_MAX_CODE;
    if (code_f < 0.0f) code_f = 0.0f;
    if (code_f > (float)DAC80501_MAX_CODE) code_f = (float)DAC80501_MAX_CODE;
    return (uint16_t)lroundf(code_f);
}

static HAL_StatusTypeDef DAC80501_SetOutputVoltage(float vout)
{
    uint16_t code = DAC80501_VoltageToCode(vout);
    return DAC80501_WriteReg(DAC80501_REG_DAC, code);
}

static HAL_StatusTypeDef DAC80501_Init(void)
{
    HAL_StatusTypeDef st;

    /* 필요 시 내부 레퍼런스/게인/버퍼 설정 */
    st = DAC80501_WriteReg(DAC80501_REG_CONFIG, 0x0000u);
    if (st != HAL_OK) return st;

    st = DAC80501_WriteReg(DAC80501_REG_GAIN, 0x0000u);
    if (st != HAL_OK) return st;

    return HAL_OK;
}

/* ========================= MCP3465R 함수 ========================= */
static HAL_StatusTypeDef MCP3465_WriteReg(uint8_t reg, uint8_t value)
{
    uint8_t tx[2];
    uint8_t rx[2] = {0};

    tx[0] = (uint8_t)(MCP3465_CMD_STATIC_WRITE | ((reg & 0x0Fu) << 2));
    tx[1] = value;

    cs_low(MCP3465_CS_GPIO_Port, MCP3465_CS_Pin);
    HAL_StatusTypeDef st = spi_txrx(&MCP3465_SPI_HANDLE, tx, rx, 2);
    cs_high(MCP3465_CS_GPIO_Port, MCP3465_CS_Pin);

    return st;
}

static HAL_StatusTypeDef MCP3465_ReadConversionRaw(int32_t *raw24)
{
    /* 단일 변환 결과 3바이트 읽기 예시 (실사용 프로토콜에 맞게 조정) */
    uint8_t tx[4] = { MCP3465_CMD_STATIC_READ, 0x00, 0x00, 0x00 };
    uint8_t rx[4] = { 0 };

    cs_low(MCP3465_CS_GPIO_Port, MCP3465_CS_Pin);
    HAL_StatusTypeDef st = spi_txrx(&MCP3465_SPI_HANDLE, tx, rx, 4);
    cs_high(MCP3465_CS_GPIO_Port, MCP3465_CS_Pin);

    if (st != HAL_OK) {
        return st;
    }

    int32_t tmp = ((int32_t)rx[1] << 16) | ((int32_t)rx[2] << 8) | (int32_t)rx[3];

    /* 24-bit signed sign-extension */
    if (tmp & 0x800000) {
        tmp |= 0xFF000000;
    }

    *raw24 = tmp;
    return HAL_OK;
}

static float MCP3465_RawToVoltage(int32_t raw24)
{
    float normalized = ((float)raw24) / MCP3465_FULL_SCALE_CODE;
    return (normalized * MCP3465_VREF) / MCP3465_PGA_GAIN;
}

static HAL_StatusTypeDef MCP3465_ReadVoltageAvg(float *vout, uint32_t samples)
{
    if ((vout == NULL) || (samples == 0u)) {
        return HAL_ERROR;
    }

    int64_t acc = 0;

    for (uint32_t i = 0; i < samples; i++) {
        int32_t raw = 0;
        HAL_StatusTypeDef st = MCP3465_ReadConversionRaw(&raw);
        if (st != HAL_OK) {
            return st;
        }
        acc += raw;
    }

    float raw_avg = (float)acc / (float)samples;
    *vout = MCP3465_RawToVoltage((int32_t)lroundf(raw_avg));
    return HAL_OK;
}

static HAL_StatusTypeDef MCP3465_Init(void)
{
    HAL_StatusTypeDef st;

    /* 아래 값은 예시. 샘플레이트, 오버샘플링, 모드 등 보드 조건에 맞게 변경 */
    st = MCP3465_WriteReg(MCP3465_REG_CONFIG0, 0xC0u); /* Vref/Clock/Shutdown bits 예시 */
    if (st != HAL_OK) return st;

    st = MCP3465_WriteReg(MCP3465_REG_CONFIG1, 0x0Cu); /* OSR 예시 */
    if (st != HAL_OK) return st;

    st = MCP3465_WriteReg(MCP3465_REG_CONFIG2, 0x8Bu); /* Gain/Boost 예시 */
    if (st != HAL_OK) return st;

    st = MCP3465_WriteReg(MCP3465_REG_CONFIG3, 0x00u); /* conversion mode 예시 */
    if (st != HAL_OK) return st;

    st = MCP3465_WriteReg(MCP3465_REG_MUX, 0x01u);     /* 채널 선택 예시 */
    if (st != HAL_OK) return st;

    return HAL_OK;
}

/* ========================= 캘리브레이션 로직 ========================= */
static HAL_StatusTypeDef run_auto_calibration(calibration_t *cal)
{
    if (cal == NULL) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef st;

    /* 1) 0.5V 인가 -> offset 점 측정 */
    st = DAC80501_SetOutputVoltage(CAL_VOLT_OFFSET_POINT);
    if (st != HAL_OK) return st;

    HAL_Delay(DAC_SETTLE_DELAY_MS);

    float meas_0p5 = 0.0f;
    st = MCP3465_ReadVoltageAvg(&meas_0p5, ADC_SAMPLES_PER_POINT);
    if (st != HAL_OK) return st;

    /* 2) 4.5V 인가 -> gain 점 측정 */
    st = DAC80501_SetOutputVoltage(CAL_VOLT_GAIN_POINT);
    if (st != HAL_OK) return st;

    HAL_Delay(DAC_SETTLE_DELAY_MS);

    float meas_4p5 = 0.0f;
    st = MCP3465_ReadVoltageAvg(&meas_4p5, ADC_SAMPLES_PER_POINT);
    if (st != HAL_OK) return st;

    /* 3) 2점 보정 계수 계산: target = a * measured + b */
    float dx = (meas_4p5 - meas_0p5);
    if (fabsf(dx) < MIN_DELTA_VOLT) {
        cal->valid = false;
        return HAL_ERROR;
    }

    float a = (CAL_VOLT_GAIN_POINT - CAL_VOLT_OFFSET_POINT) / dx;
    float b = CAL_VOLT_OFFSET_POINT - (a * meas_0p5);

    cal->a_gain = a;
    cal->b_offset = b;
    cal->raw_at_0p5v = meas_0p5;
    cal->raw_at_4p5v = meas_4p5;
    cal->valid = true;

    return HAL_OK;
}

static float apply_calibration(float measured_voltage, const calibration_t *cal)
{
    if ((cal == NULL) || (!cal->valid)) {
        return measured_voltage;
    }
    return (cal->a_gain * measured_voltage) + cal->b_offset;
}

/* ========================= 공개 API 예시 ========================= */
HAL_StatusTypeDef APP_AutoCal_InitAndRun(void)
{
    HAL_StatusTypeDef st;

    st = DAC80501_Init();
    if (st != HAL_OK) return st;

    st = MCP3465_Init();
    if (st != HAL_OK) return st;

    st = run_auto_calibration(&g_cal);
    if (st != HAL_OK) return st;

    return HAL_OK;
}

HAL_StatusTypeDef APP_ReadCalibratedVoltage(float *voltage_calibrated)
{
    if (voltage_calibrated == NULL) {
        return HAL_ERROR;
    }

    float v_meas = 0.0f;
    HAL_StatusTypeDef st = MCP3465_ReadVoltageAvg(&v_meas, ADC_SAMPLES_PER_POINT);
    if (st != HAL_OK) {
        return st;
    }

    *voltage_calibrated = apply_calibration(v_meas, &g_cal);
    return HAL_OK;
}

const calibration_t* APP_GetCalibrationData(void)
{
    return &g_cal;
}

/* ========================= main.c 사용 예시 =========================
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_SPI2_Init();

    if (APP_AutoCal_InitAndRun() != HAL_OK) {
        // error handling
    }

    while (1) {
        float v_cal = 0.0f;
        if (APP_ReadCalibratedVoltage(&v_cal) == HAL_OK) {
            // v_cal 사용
        }
        HAL_Delay(100);
    }
}
*/
