# STM32L562 + ESP32-C3-WROOM (AT command)

STM32L562CET6가 UART로 ESP32-C3-WROOM의 ESP-AT 펌웨어를 제어해 Wi-Fi AP와 원격 TCP/UDP 서버에 접속하는 예제입니다. 여기서 `server_ip`/`server_port`는 ESP32가 접속할 **원격 서버** 주소입니다.

## STM32CubeIDE/CubeMX 설정

1. ESP32-C3에 Espressif ESP-AT 펌웨어를 설치하고 기본 UART 핀/baud를 확인합니다.
2. STM32 UART TX→ESP RX, STM32 RX←ESP TX, GND를 공통 연결합니다. 전원과 EN 핀은 모듈 데이터시트에 맞게 구성합니다(3.3 V 로직).
3. STM32CubeIDE의 `.ioc` 설정에서 **Connectivity → USART1 → Asynchronous**를 선택하고 115200 baud, 8 data bits, no parity, 1 stop bit, TX/RX 모드로 설정합니다.
4. **NVIC Settings → USART1 global interrupt**를 활성화한 뒤 코드를 생성합니다. CubeMX가 `Core/Inc/usart.h`, `Core/Src/usart.c`와 전역 핸들 `huart1`을 생성합니다.
5. `esp32c3_at.c/.h`, `app_wifi.c/.h`를 프로젝트의 `Core/Src`, `Core/Inc`에 각각 추가하고 `app_wifi.c`의 AP/서버 설정값을 수정합니다.

`main.c`의 `/* USER CODE BEGIN Includes */` 영역에 다음을 추가합니다.

```c
#include "app_wifi.h"
```

CubeMX가 생성한 `MX_USART1_UART_Init()` 호출 **다음**의 `/* USER CODE BEGIN 2 */` 영역에서 초기화합니다.

```c
if (!App_WifiInit()) {
    Error_Handler();
}
```

`while (1)`의 `/* USER CODE BEGIN WHILE */` 또는 `/* USER CODE BEGIN 3 */` 영역에서 상태 머신을 계속 실행합니다.

```c
while (1) {
    App_WifiProcess();

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
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    App_WifiUartErrorCallback(huart);
}
```

`app_wifi.c`는 USART1의 1-byte interrupt 수신을 circular buffer에 저장하므로 AT 명령을 기다리는 동안 들어오는 비동기 메시지도 유실 가능성을 줄입니다. `HAL_UART_RxCpltCallback()` 안에서 반드시 다음 수신을 재등록합니다.

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
- SSID/비밀번호에 `"`, `,`, `\\` 같은 AT 특수문자가 있으면 ESP-AT escaping 규칙에 맞춰 별도 이스케이프 처리가 필요합니다.
- AT 펌웨어 버전에 따라 명령 지원/응답이 다를 수 있으므로 실제 펌웨어의 ESP-AT 명령 문서를 확인하십시오.
- `+IPD` 데이터 파싱은 애플리케이션 프로토콜에 맞게 별도로 추가해야 합니다.
