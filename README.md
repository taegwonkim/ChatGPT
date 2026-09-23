# STM32L562 + ESP32-C3-WROOM AT 폴링 예제

STM32L562CET6가 UART 폴링 방식으로 ESP32-C3-WROOM의 ESP-AT 펌웨어를 제어하는
예제입니다. 초기 통신 확인, Station 모드 설정, MAC 주소 확인, DHCP/고정 IP 설정,
AP 접속, TCP 서버 접속 및 접속 단절 시 재시도를 상태 머신으로 처리합니다.

> ESP32-C3-WROOM은 출고 상태에 따라 ESP-AT 펌웨어가 없을 수 있습니다. 먼저
> Espressif의 ESP-AT 펌웨어를 모듈에 플래시하고, 펌웨어의 UART 핀/baud rate를
> 확인하십시오. 이 코드는 기본 AT 명령 집합을 사용합니다.

## CubeMX 설정

1. MCU로 **STM32L562CETx**를 선택합니다.
2. `USART1`을 Asynchronous로 켜고 ESP-AT 펌웨어 설정과 동일하게 구성합니다.
   일반적인 시작값은 `115200, 8 data bits, no parity, 1 stop bit, no flow control`입니다.
3. USART TX/RX GPIO의 alternate function을 설정하고 STM32 TX를 ESP RX에, STM32 RX를
   ESP TX에 교차 연결합니다. 두 장치의 GND도 연결합니다. 신호 전압은 3.3 V입니다.
4. ESP32-C3의 전원은 송신 순간 전류를 감당하는 별도 3.3 V 전원을 권장합니다.
5. NVIC UART interrupt와 DMA는 필요하지 않습니다. SysTick은 HAL 기본 1 ms를 유지합니다.
6. CubeIDE 프로젝트 생성 후 `esp32_at.c`, `wifi_app.c`를 빌드에 포함하고 헤더를
   `Core/Inc`에 둡니다. `main_example.c` 자체는 **빌드에서 제외**합니다.

## main.c 연결

`main_example.c`의 설정 객체와 두 USER CODE 블록을 CubeMX가 만든 `main.c`의 같은
위치에 복사합니다. DHCP를 사용하려면 `.dhcp_enabled = true`로 둡니다. 고정 IP는
`false`로 바꾸고 `static_ip`, `gateway`, `netmask`를 실제 네트워크에 맞춥니다.

```c
/* USER CODE BEGIN 2 */
WifiApp_Init(&wifi, &huart1, &wifi_config);
/* USER CODE END 2 */

/* USER CODE BEGIN WHILE */
while (1)
{
    WifiApp_Process(&wifi); /* AT UART는 이 함수 안에서 폴링됩니다. */
    HAL_Delay(10U);
}
/* USER CODE END WHILE */
```

초기화가 성공하면 `wifi.mac`에 `aa:bb:cc:dd:ee:ff` 형식의 Station MAC 주소가
저장됩니다. `WifiApp_IsOnline(&wifi)`로 AP와 TCP 서버 연결 완료 상태를 확인할 수
있습니다. `health_check_ms`마다 `AT+CWSTATE?`와 `AT+CIPSTATUS`를 확인하고 AP 단절은
AP부터, 서버 단절은 TCP 서버부터 `retry_ms` 간격으로 재접속합니다. 각 AT 명령은
응답을 기다리는 동안 블로킹되므로, 엄격한 실시간 작업은 별도 RTOS task 또는
인터럽트 기반 드라이버로 분리해야 합니다.

## 사용 명령 순서

`AT` → `ATE0` → `AT+RST` (`ready` 대기) → `AT` → `ATE0` → `AT+CWMODE=1` →
`AT+CIPMUX=0` → `AT+CIPSTAMAC?` →
`AT+CWDHCP` (고정 IP이면 `AT+CIPSTA` 추가) → `AT+CWJAP` → `AT+CIPSTART`.

SSID/암호/IP에 큰따옴표, 역슬래시 또는 개행 문자는 안전을 위해 거부합니다.
제품에서는 자격 증명을 소스에 하드코딩하지 말고 보호된 설정 저장소에서 읽으십시오.
