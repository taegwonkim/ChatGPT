# STM32 bare-metal ESP32-C3 ESP-AT driver

STM32CubeIDE/CubeMX 프로젝트에서 RTOS 없이 ESP32-C3의 공식 ESP-AT 펌웨어를
UART로 제어하는 작은 C 드라이버입니다. 명령 API는 동기식이며, 수신은 1바이트
UART 인터럽트와 링 버퍼를 사용합니다. `+IPD` TCP 데이터는 별도 콜백으로
전달됩니다.

## 전제 조건

- ESP32-C3에 ESP-AT 펌웨어가 설치되어 있어야 합니다.
- STM32와 ESP32-C3 UART 전압은 3.3 V이며 GND를 공통으로 연결합니다.
- 기본 예제는 STM32의 `USART1`을 사용합니다. CubeMX에서 핀, baud rate(ESP-AT
  설정과 동일, 보통 115200), 8-N-1 및 USART global interrupt를 설정하십시오.
- 드라이버는 STM32 HAL과 `HAL_GetTick()`에 의존합니다. RTOS는 필요하지 않습니다.

## ESP32-C3-DevKit 연결: USB가 아니라 UART

이 드라이버는 **DevKit의 USB 커넥터가 아니라 ESP-AT 명령용 UART 핀**에
연결합니다. USB 커넥터는 ESP32-C3 펌웨어 다운로드, PC 콘솔/로그 및 보드
전원 공급에 사용할 수 있지만, 이 STM32 HAL 드라이버는 USB host나 USB CDC를
구현하지 않습니다.

기본 배선은 다음과 같이 교차 연결합니다.

| STM32 | ESP32-C3-DevKit |
| --- | --- |
| UART TX | ESP-AT UART RX |
| UART RX | ESP-AT UART TX |
| GND | GND |
| 선택 사항 GPIO | EN/RESET |

두 보드는 반드시 3.3 V UART 논리 레벨을 사용해야 합니다. DevKit에 USB 전원을
공급하면서 STM32 UART를 연결할 수 있지만 GND는 공통으로 연결하고, 두 보드의
3.3 V 출력 핀을 서로 직접 연결해 동시에 전원을 공급하지 마십시오.

ESP-AT UART의 실제 GPIO 번호는 DevKit 이름만으로 결정하면 안 됩니다. 사용하는
ESP-AT 바이너리의 대상 모듈과 빌드 설정에 따라 AT command UART와 핀 매핑이
달라질 수 있으므로 해당 펌웨어의 pin configuration을 확인하십시오. 직접
빌드한다면 ESP-AT의 UART 설정에서 선택한 TX/RX 핀을 위 표대로 연결합니다.

일부 DevKit의 USB-to-UART bridge는 ESP32-C3의 UART0에 이미 연결되어 있습니다.
AT command UART도 같은 UART0 핀을 사용하도록 구성한 경우 PC USB-UART와 STM32가
동시에 송신하면 전기적 충돌과 데이터 혼선이 발생할 수 있습니다. 이 경우에는
다음 중 하나를 선택하십시오.

1. USB는 분리하고 DevKit을 별도의 적절한 전원으로 공급한 뒤 STM32 UART만
   연결합니다.
2. USB를 전원용으로만 사용하되 USB-UART bridge가 해당 신호를 구동하지 않는지
   보드 회로도를 확인합니다.
3. 가장 확실한 방법으로 ESP-AT command UART를 bridge와 겹치지 않는 GPIO/UART로
   설정하여 다시 빌드합니다.

즉, **펌웨어를 굽고 PC에서 로그를 확인할 때는 USB**, 실제 애플리케이션에서
**STM32가 AT 명령을 보낼 때는 3.3 V UART TX/RX/GND**를 사용합니다.

## CubeIDE 적용

1. `Inc/esp_at.h`를 프로젝트의 `Core/Inc`로, `Src/esp_at.c`를 `Core/Src`로
   복사합니다.
2. CubeMX에서 사용할 UART를 활성화하고 NVIC의 UART global interrupt를
   활성화합니다.
3. `Example/main_integration.c`를 참고하여 전역 핸들, 두 HAL 콜백 및 초기화
   코드를 `main.c`의 `USER CODE` 영역에 추가합니다.
4. `MX_USART1_UART_Init()` 다음에 `app_esp_start()`를 호출합니다.
5. 메인 `while (1)`에서 `ESP_AT_Process()`를 계속 호출합니다. 긴 사용자 작업은
   수신 링 버퍼가 넘치지 않도록 나누어 실행하십시오.

CubeMX가 생성하는 `stm32xx_it.c`의 UART IRQ handler는 일반적으로
`HAL_UART_IRQHandler(&huart1)`를 이미 호출합니다. 직접 만든 프로젝트라면 이
호출도 확인해야 합니다.

## 제공 API

- `ESP_AT_Command`: 임의의 AT 명령 실행
- `ESP_AT_Reset`, `ESP_AT_SetStationMode`, `ESP_AT_JoinAP`
- `ESP_AT_TcpConnect`, `ESP_AT_Send`, `ESP_AT_Close`
- line callback: 상태/비동기 메시지 수신
- data callback: `+IPD[,link_id],length:` 뒤의 TCP payload 수신

모든 명령 API는 timeout 동안 busy-wait하지만 그 안에서 수신 데이터를 계속
처리합니다. 콜백은 메인 컨텍스트의 `ESP_AT_Process()`에서 실행되므로 ISR에서
복잡한 작업을 하지 않습니다. 콜백으로 전달된 버퍼는 콜백 반환 후 보관되지
않으므로 필요한 데이터는 즉시 복사하십시오.

## 메모리와 제약

`ESP_AT_RX_BUFFER_SIZE`, `ESP_AT_LINE_BUFFER_SIZE`, `ESP_AT_IPD_CHUNK_SIZE`를
컴파일 정의 또는 헤더에서 조절할 수 있습니다. 수신 링 버퍼가 가득 차면 가장
최근 바이트를 버리고 `ESP_AT_OVERFLOW`를 반환합니다.

현재 구현은 단일 TCP 연결(`CIPMUX=0`) 기준입니다. TLS, UDP, 다중 연결,
transparent mode 및 Wi-Fi 자동 재연결 정책은 애플리케이션 요구사항에 맞게
추가해야 합니다. 제품에서는 ESP-AT 펌웨어 버전에 맞는 명령 문서를 확인하고
하드웨어 reset/enable 핀과 watchdog 복구도 함께 구성하는 것을 권장합니다.
