# STM32L562 + ESP32-C3-WROOM (AT command)

Windows용 Visual C# 설정/모니터 프로그램은 [`pc/README.md`](pc/README.md)를 참조하십시오.

STM32L562CET6가 UART로 ESP32-C3-WROOM의 ESP-AT 펌웨어를 제어해 Wi-Fi AP와 원격 TCP/UDP 서버에 접속하는 예제입니다. 여기서 `server_ip`/`server_port`는 ESP32가 접속할 **원격 서버** 주소입니다.

## STM32CubeIDE/CubeMX 설정

1. ESP32-C3에 Espressif ESP-AT 펌웨어를 설치하고 기본 UART 핀/baud를 확인합니다.
2. STM32 UART TX→ESP RX, STM32 RX←ESP TX, GND를 공통 연결합니다. 전원과 EN 핀은 모듈 데이터시트에 맞게 구성합니다(3.3 V 로직).
3. `.ioc`에서 USART1(ESP32), USART2(외부 ADC), USART3(PC)를 모두 **Asynchronous** TX/RX로 설정합니다. 각 장치의 baud rate에 맞추되 ESP32는 기본적으로 115200, 8-N-1입니다.
4. **NVIC Settings**에서 USART1, USART2, USART3 global interrupt를 활성화합니다. USART3 RX interrupt는 PC의 reset 설정 명령을 받는 데 사용합니다.
5. 코드를 생성해 `usart.h/.c`에 `huart1`, `huart2`, `huart3`와 `MX_USART1/2/3_UART_Init()`가 생성되었는지 확인합니다.
6. CubeMX에서 **RTC**를 활성화하고 WakeUp interrupt를 NVIC에서 활성화하여 `rtc.h/.c`, `hrtc`, `MX_RTC_Init()`가 생성되게 합니다. RTC clock source는 LSE 사용을 권장합니다.
7. `esp32c3_at.c/.h`, `app_wifi.c/.h`, `app_data.c/.h`, `app_reset.c/.h`를 프로젝트의 `Core/Src`, `Core/Inc`에 각각 추가하고 `app_wifi.c`의 AP/서버 설정값을 수정합니다.

`main.c`의 `/* USER CODE BEGIN Includes */` 영역에 다음을 추가합니다.

```c
#include "app_wifi.h"
#include "app_data.h"
#include "app_reset.h"
```

CubeMX가 생성한 세 USART 초기화 호출 **다음**의 `/* USER CODE BEGIN 2 */` 영역에서 초기화합니다.

```c
if (!App_WifiInit()) {
    Error_Handler();
}
if (!App_DataInit()) {
    Error_Handler();
}
if (!App_ResetInit()) { /* MX_RTC_Init(), MX_USART3_UART_Init() 다음에 호출 */
    Error_Handler();
}
```

`while (1)`의 `/* USER CODE BEGIN WHILE */` 또는 `/* USER CODE BEGIN 3 */` 영역에서 상태 머신을 계속 실행합니다.

```c
while (1) {
    App_WifiProcess();
    App_DataProcess();
    App_ResetProcess();

    if (App_WifiIsOnline()) {
        /* 주기 타이머/플래그 조건에서만 App_WifiSend() 호출 */
    }
}
```

마지막으로 `main.c`의 USER CODE 영역에 HAL callback을 추가합니다. 다른 UART도 사용한다면 기존 callback을 삭제하지 말고 아래 전달 함수 호출을 합칩니다.

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    App_WifiUartRxCpltCallback(huart);
    App_DataUartRxCpltCallback(huart);
    App_ResetUartRxCpltCallback(huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    App_WifiUartErrorCallback(huart);
    App_DataUartErrorCallback(huart);
    App_ResetUartErrorCallback(huart);
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    App_ResetRtcWakeupCallback(hrtc);
}
```

`app_wifi.c`는 USART1의 AT 응답을 circular buffer에 저장합니다. `app_data.c`는 USART2에서 newline(`\n`) 단위 ADC 프레임을 받고, 가장 최근 프레임을 매 1초마다 USART3 PC로 보냅니다. Wi-Fi가 online이면 같은 프레임을 서버에도 보냅니다. Wi-Fi AT 응답을 기다리는 중에도 background callback이 `App_DataProcess()`를 호출하므로 AP/서버가 끊어진 동안 USART2 수신과 USART3 주기 송신은 계속됩니다.

외부 ADC가 newline이 아닌 고정 길이 또는 바이너리 프로토콜을 사용한다면 `App_DataUartRxCpltCallback()`의 frame 판별부를 변경하거나, 완성된 프레임에 대해 `App_DataSetLatestFromISR()`를 호출하십시오.

## RTC 주기 software reset

PC는 선택한 단위에 따라 USART3 명령을 구분해서 전송합니다. 분 단위는
`<STX>RTC_R_M<CR><LF>` / `<STX>RTC_W_M,minutes<CR><LF>`, 시간 단위는
`<STX>RTC_R_H<CR><LF>` / `<STX>RTC_W_H,hours<CR><LF>`입니다. MCU 응답도 각각
`<STX>RTC_R_M,minutes<CR><LF>` 또는 `<STX>RTC_R_H,hours<CR><LF>`입니다. 값 `0`은
자동 reset OFF이며 최대값은 525600분 또는 8760시간(365일)입니다. 내부에서는 초로
변환해 RTC timer를 설정하고 backup register에 저장하므로 software reset 후에도 유지됩니다.

RTC wake-up timer의 16-bit 한계를 넘는 주기는 최대 65536초 단위로 나눠 예약합니다.
주기가 끝나면 RTC wake-up interrupt에서 `NVIC_SystemReset()`을 호출합니다. 전원 제거 후에도
설정을 유지하려면 VBAT 및 backup domain 전원 조건을 하드웨어 설계에 맞게 구성해야 합니다.

## 명령 절차

상태 머신은 다음 순서로 실행합니다.

1. `AT+RST`, `ATE0`, `AT+CWMODE=1`, `AT+CIPMUX=0`
2. DHCP 사용: `AT+CWDHCP=1,1`
3. DHCP 미사용: `AT+CWDHCP=1,0` 후 `AT+CIPSTA="ip","gateway","netmask"`
4. `AT+CWJAP="ssid","password"`
5. `AT+CIPSTART="TCP","server_ip",port` (UDP 선택 가능)
6. 송신 시 `AT+CIPSEND=length`, `>` 확인, payload 전송, `SEND OK` 확인

`WIFI DISCONNECT`가 수신되면 AP 재접속 대기 상태로, `CLOSED` 또는 송신 실패가 발생하면 서버 재접속 대기 상태로 전환합니다. 대기 시간은 각각 `ap_retry_ms`, `server_retry_ms`입니다. AP가 끊기면 서버 연결도 무효화합니다.

## 주의 사항

- 제공된 어댑터는 USART1 interrupt circular buffer를 사용합니다. 통신량이 많으면 DMA circular mode와 IDLE line 방식으로 교체하는 것을 권장합니다.
- USART2 ADC 예제는 한 프레임이 `\n`으로 끝난다고 가정하며 최대 길이는 128 bytes입니다. `ADC_FRAME_MAX`와 parser를 실제 ADC 프로토콜에 맞게 변경하십시오.
- SSID/비밀번호에 `"`, `,`, `\\` 같은 AT 특수문자가 있으면 ESP-AT escaping 규칙에 맞춰 별도 이스케이프 처리가 필요합니다.
- AT 펌웨어 버전에 따라 명령 지원/응답이 다를 수 있으므로 실제 펌웨어의 ESP-AT 명령 문서를 확인하십시오.
- `+IPD` 데이터 파싱은 애플리케이션 프로토콜에 맞게 별도로 추가해야 합니다.
