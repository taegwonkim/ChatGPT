# STM32L562 + ESP32-C3-WROOM (AT command)

STM32L562CET6가 UART로 ESP32-C3-WROOM의 ESP-AT 펌웨어를 제어해 Wi-Fi AP와 원격 TCP/UDP 서버에 접속하는 예제입니다. 여기서 `server_ip`/`server_port`는 ESP32가 접속할 **원격 서버** 주소입니다.

## 연결 및 준비

1. ESP32-C3에 Espressif ESP-AT 펌웨어를 설치하고 기본 UART 핀/baud를 확인합니다.
2. STM32 UART TX→ESP RX, STM32 RX←ESP TX, GND를 공통 연결합니다. 전원과 EN 핀은 모듈 데이터시트에 맞게 구성합니다(3.3 V 로직).
3. CubeMX에서 해당 UART를 115200, 8 data bits, no parity, 1 stop bit로 설정합니다.
4. `esp32c3_at.c/.h`를 빌드에 추가하고 `example_stm32l562.c`의 UART 핸들 및 접속 정보를 수정합니다.
5. 초기화 뒤 `while (1)`에서 `App_Loop()`를 계속 호출합니다.

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

- 제품에서는 blocking HAL 수신 대신 UART DMA/인터럽트 circular buffer를 권장합니다. 제공된 HAL 어댑터는 절차를 명확히 보여 주는 단순 예제입니다.
- SSID/비밀번호에 `"`, `,`, `\\` 같은 AT 특수문자가 있으면 ESP-AT escaping 규칙에 맞춰 별도 이스케이프 처리가 필요합니다.
- AT 펌웨어 버전에 따라 명령 지원/응답이 다를 수 있으므로 실제 펌웨어의 ESP-AT 명령 문서를 확인하십시오.
- `+IPD` 데이터 파싱은 애플리케이션 프로토콜에 맞게 별도로 추가해야 합니다.
