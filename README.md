# STM32 + ESP32-C3 AT Wi-Fi/TCP 예제

STM32가 UART로 ESP32-C3의 **ESP-AT 펌웨어**를 제어하여 Wi-Fi에 접속하고,
PC의 TCP 서버에 연결한 뒤 데이터를 주고받는 전체 예제입니다. 연결이 끊어지면
비차단 상태 머신이 Wi-Fi 상태부터 확인하여 자동으로 다시 연결합니다.

> 이 저장소의 코드는 특정 STM32 보드에 종속되지 않습니다. CubeMX가 생성한
> `UART_HandleTypeDef`와 `HAL_GetTick()`을 사용하므로 MCU/보드에 맞게 핀과 UART
> 인스턴스만 선택하면 됩니다.

## 1. 전체 구성

```text
STM32  <-- UART (115200, 8-N-1) -->  ESP32-C3  <-- Wi-Fi -->  AP/router
                                                               |
                                                        TCP port 5000
                                                               |
                                                            Server PC
```

1. STM32는 `AT`로 모듈 응답을 확인합니다.
2. `ATE0`, `AT+CWMODE=1`, `AT+CIPMUX=0`으로 echo를 끄고 station/single connection
   모드로 설정합니다.
3. `AT+CWJAP="SSID","PASSWORD"`로 AP에 접속합니다.
4. `AT+CIPSTART="TCP","PC_IP",5000`으로 PC 서버에 연결합니다.
5. 송신 시 `AT+CIPSEND=<길이>` 후 `>` 프롬프트가 오면 raw payload를 보냅니다.
6. 수신 데이터는 ESP-AT의 `+IPD,<길이>:<payload>` 형식에서 분리됩니다.
7. `CLOSED`, Wi-Fi disconnect, 명령 timeout/error를 감지하면 지수형 backoff 후
   재시도합니다. 1, 2, 4, 8초로 증가하며 최대 30초입니다.

## 2. 하드웨어 연결

| STM32 | ESP32-C3 | 설명 |
|---|---|---|
| UART TX | UART RX | 교차 연결 |
| UART RX | UART TX | 교차 연결 |
| GPIO output (선택) | EN | active-high reset/enable |
| 3.3 V 공급 | 3V3 | 모듈의 순간 전류를 감당할 안정된 전원 사용 |
| GND | GND | 반드시 공통 접지 |

ESP32-C3 I/O는 3.3 V 논리입니다. 5 V UART를 직접 연결하지 마십시오. 개발보드의
기본 AT UART 핀/baud rate는 보드와 ESP-AT 이미지 설정에 따라 다를 수 있으므로
펌웨어 문서를 확인하십시오. ESP32-C3에는 사전에 ESP-AT 펌웨어가 설치되어 있어야
합니다. 터미널에서 `AT\r\n`에 `OK`가 반환되는지 먼저 확인하는 것이 좋습니다.

## 3. STM32CubeMX 설정

1. **Connectivity > USARTx**를 `Asynchronous`로 활성화합니다.
2. Parameter Settings를 **115200 bit/s, 8 data bits, no parity, 1 stop bit**로
   설정합니다. ESP-AT의 실제 설정이 다르면 그 값에 맞춥니다.
3. NVIC Settings에서 **USART global interrupt**를 활성화합니다.
4. (권장) ESP EN 핀을 GPIO Output으로 지정하고 초기값을 High로 둡니다.
5. Project Manager에서 STM32CubeIDE 프로젝트를 생성합니다.
6. `Core/Inc`에 `esp32_at.h`, `Core/Src`에 `esp32_at.c`를 추가하거나 이 저장소의
   파일을 프로젝트로 복사합니다.

UART 수신은 1바이트 interrupt 방식으로 작성되어 이해하기 쉽습니다. 높은 처리량이
필요하면 `HAL_UARTEx_ReceiveToIdle_DMA()`와 ring buffer로 교체하십시오. ISR에서는
수신 바이트를 전달하고 즉시 다음 interrupt를 걸 뿐, 명령 처리나 로그 출력을 하지
않습니다.

## 4. 애플리케이션 연결

`examples/stm32_main_snippet.c`의 USER CODE 블록을 CubeMX가 만든 `main.c`의 같은
위치에 옮깁니다. 다음 설정은 반드시 환경에 맞게 바꾸십시오.

```c
#define WIFI_SSID      "YOUR_AP_SSID"
#define WIFI_PASSWORD  "YOUR_AP_PASSWORD"
#define SERVER_IP      "192.168.0.10"  /* 서버 PC의 LAN 주소 */
#define SERVER_PORT    5000
```

`SERVER_IP`에 `127.0.0.1`을 쓰면 ESP 자신을 뜻하므로 연결되지 않습니다. Windows는
`ipconfig`, Linux/macOS는 `ip addr`/`ifconfig`로 Wi-Fi 인터페이스의 주소를 확인하고,
OS 방화벽에서 TCP 5000 인바운드를 허용합니다. AP의 client isolation이 켜져 있으면
동일 Wi-Fi 단말 간 통신이 막힐 수 있습니다.

메인 루프에서는 항상 `ESP_AT_Process()`를 빠르게 반복 호출해야 합니다. 연결 후
`ESP_AT_Send()`를 호출하면 전송을 시작하며, 반환값이 `false`이면 아직 연결되지
않았거나 이전 송신 중이므로 나중에 다시 호출해야 합니다. 수신 callback의 `data`는
NUL 종료 문자열이 아니라 길이가 지정된 binary 데이터입니다.

## 5. PC 서버 실행

Python 3만 있으면 됩니다.

```bash
python3 tools/tcp_server.py --host 0.0.0.0 --port 5000
```

서버는 여러 클라이언트를 처리하고 수신 bytes를 출력한 뒤 `ACK:`를 붙여 돌려줍니다.
연결/재연결 시험은 서버를 종료했다 다시 실행하거나 AP를 잠시 끄면 됩니다.

수동 시험에는 다음 명령도 쓸 수 있습니다.

```bash
printf 'hello\n' | nc SERVER_PC_IP 5000
```

## 6. 상태와 복구 정책

| 상태 | 동작 | 성공 시 | 실패/timeout 시 |
|---|---|---|---|
| `SYNC` | `AT` | 초기 설정 | backoff |
| `CONFIG` | echo/mode/mux 설정 | Wi-Fi 연결 | backoff |
| `JOIN_AP` | `CWJAP` | TCP 연결 | backoff |
| `CONNECT_TCP` | `CIPSTART` | online | backoff |
| `ONLINE` | `CIPSEND`, `+IPD` 처리 | 유지 | `CLOSED`면 backoff |
| `BACKOFF` | 일정 시간 대기 | sync 재시작 | 대기 증가 |

TCP가 끊기면 애플리케이션 수준 메시지 경계와 전달 보장은 사라집니다. 중요한 데이터는
메시지 ID/길이/checksum 및 server ACK를 정의하고, ACK되지 않은 메시지를 STM32의
queue에 보관해 재연결 후 재전송해야 합니다. 동일 ID가 재전송될 수 있으므로 서버도
중복 제거를 구현하는 것이 안전합니다.

## 7. 문제 해결

- `AT` 응답 없음: TX/RX 교차, 공통 GND, baud, ESP-AT 설치 여부를 확인합니다.
- `CWJAP` 실패: SSID/password의 `"`, `\\`, 제어문자는 이 간단한 예제에서 지원하지
  않습니다. 2.4 GHz AP인지와 신호 세기를 확인합니다.
- `CIPSTART` timeout: PC IP/port, server bind 주소, 방화벽, AP isolation을 확인합니다.
- 간헐적 reset: ESP32-C3 전원 공급과 decoupling을 확인합니다.
- `RX overflow`: `ESP_AT_RX_BUFFER_SIZE`를 늘리거나 DMA ring buffer로 전환합니다.

## 파일

- `Core/Inc/esp32_at.h`: 공개 API와 설정 구조체
- `Core/Src/esp32_at.c`: AT 상태 머신, 재연결, `+IPD` parser
- `examples/stm32_main_snippet.c`: CubeMX/HAL 통합 예제
- `tools/tcp_server.py`: PC용 echo/ACK TCP server

