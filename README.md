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
