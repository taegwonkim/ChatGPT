# STM32L562 측정 게이트웨이 예제

대상은 **STM32L562CEU6 + FreeRTOS(CMSIS-RTOS2)** 이며 다음 데이터 흐름을 구현하는
CubeIDE용 애플리케이션 골격이다.

```text
FPGA --USART2--> STM32 --USART1/AT--> ESP32-C3-WROOM --> TCP server
                       |
                       +--USART3/RS-485 또는 USB CDC--> PC
                       +--SPI2--> W25Q40CLSNIG
```

> 이 저장소에는 CubeMX가 생성하는 HAL/시작 코드가 포함되어 있지 않다. 먼저 아래와
> 같이 `.ioc` 프로젝트를 생성한 뒤 `App/`를 프로젝트에 추가한다. 실제 핀, 클록,
> RS-485 DE 극성 및 FPGA 프레임은 회로/FPGA 사양에 맞춰 확정해야 한다.

## 1. 권장 핀/주변장치 설정

핀은 패키지/PCB에 따라 달라지므로 CubeMX에서 충돌 없는 AF를 선택한다.

| 블록 | CubeMX 설정 | 권장 파라미터 |
|---|---|---|
| USART1 / ESP32 | Asynchronous, TX/RX | 115200, 8-N-1, HW flow control 없음 |
| USART2 / FPGA | Asynchronous, TX/RX | 115200, 8-N-1 (FPGA와 일치) |
| USART3 / PC | Asynchronous, TX/RX | 115200, 8-N-1; RS-485 DE GPIO 추가 |
| SPI2 / Flash | Full Duplex Master | 8 bit, MSB, mode 0, NSS software, 초기 prescaler로 SCK 10 MHz 이하 |
| USB | USB device FS + CDC | USB clock 48 MHz, VBUS sensing은 회로에 맞춤 |
| GPIO | FLASH_CS, RS485_DE | 초기값 CS=High, DE=Low |
| Watchdog | IWDG | 약 4 s (제품 요구사항에 맞춰 산정) |

W25Q40의 `/WP`, `/HOLD`는 사용하지 않으면 풀업하고 3.3 V 전원에 100 nF를 둔다.
ESP32와 MCU는 3.3 V UART/GND를 공통으로 연결한다. RS-485에는 별도 transceiver와
종단/바이어스를 사용하며 USART 핀을 케이블에 직접 연결하지 않는다.

DMA는 USART1 RX/TX, USART2 RX/TX, USART3 RX/TX에 각각 추가한다. RX DMA mode는
**Circular**, TX는 **Normal**로 설정한다. 각 USART global interrupt와 DMA interrupt를
활성화하고, `HAL_UARTEx_ReceiveToIdle_DMA()` 사용 시 DMA half-transfer interrupt는
비활성화하는 것을 권장한다. SPI2는 작은 제어 트랜잭션이므로 이 예제에서 polling을
사용한다.

## 2. FreeRTOS 설정 (상세)

CubeMX의 **Middleware > FreeRTOS**에서 CMSIS_V2를 선택한다.

* `configUSE_PREEMPTION=1`, tick 1000 Hz, `configUSE_TIME_SLICING=1`
* heap 구현은 `heap_4`, `TOTAL_HEAP_SIZE`는 우선 32 KiB
* `configCHECK_FOR_STACK_OVERFLOW=2`, malloc failed hook 활성화
* mutex/recursive mutex/counting semaphore/software timer 활성화
* newlib reentrant는 printf가 필요할 때만 활성화(태스크당 RAM 증가)
* NVIC priority grouping은 4, `LIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY=5` 권장
  (RTOS API를 호출하는 ISR 우선순위 숫자는 반드시 5 이상)

CubeMX 기본 task는 제거하거나 `StartDefaultTask`에서 `App_RTOS_Init()`만 호출 후
자기 자신을 종료한다. 권장 객체는 다음과 같다(코드에서 정적으로 생성).

| 객체 | 우선순위 | stack | 역할 |
|---|---:|---:|---|
| Supervisor | AboveNormal | 768 words | 시작 명령, Wi-Fi 상태/재접속 |
| FpgaRx | High | 512 words | USART2 프레임 검증/측정 큐 입력 |
| Router | Normal | 768 words | 측정값을 Wi-Fi/PC 출력 큐로 fan-out |
| EspAt | Normal | 1024 words | ESP AT 명령 직렬화 및 TCP 송신 |
| Pc | BelowNormal | 768 words | RS-485/USB 설정 명령 및 측정 송신 |
| Flash | Low | 768 words | 설정/판정치 flash 쓰기(단일 소유자) |

ISR에서는 바이트를 해석하거나 flash/AT 명령을 실행하지 말고 DMA idle 이벤트에서
`App_UartRxEventFromISR()`로 복사/통지만 한다. 같은 UART의 AT 명령은 오직 EspAt task가
소유한다. 측정 큐가 가득 차면 가장 오래된 값을 버리도록 하여 수신 task가 막히지
않게 한다.

## 3. CubeMX 코드 연결

CubeMX가 초기화(`MX_USARTx_UART_Init`, `MX_SPI2_Init`, `MX_USB_DEVICE_Init`)를 마친 뒤,
커널 시작 전에 다음을 호출한다.

```c
App_Init(&huart1, &huart2, &huart3, &hspi2);
App_RTOS_Init();
osKernelStart();
```

CubeMX의 `usbd_cdc_if.c` 수신 callback에는 다음을 추가한다.

```c
App_UsbRx(Buf, *Len);
USBD_CDC_SetRxBuffer(&hUsbDeviceFS, Buf);
USBD_CDC_ReceivePacket(&hUsbDeviceFS);
```

UART DMA 초기 수신과 callback 연결 예시는 `App/app_port.c`에 있다. `app_port.c`의
weak 함수(`Board_FlashCs`, `Board_Rs485De`, `Board_UsbTransmit`)를 실제 GPIO/CDC 코드로
override한다.

## 4. PC/FPGA 프로토콜

텍스트 줄(`\n`, 최대 255 byte)을 사용하여 bring-up과 디버깅을 쉽게 했다.

* PC 설정: `CFG,{"ssid":"AP","password":"secret","ip":"192.168.0.10","port":5000,"dhcp":true}`
* 설정 조회: `GETCFG`
* FPGA 시작: `START\r\n`
* FPGA 측정: `MEAS,<sequence>,<signed_value>\n`
* MCU 출력: `MEAS,<sequence>,<signed_value>,<unix_or_uptime_ms>\n`

제품에서는 JSON parser를 정식 라이브러리로 교체하거나 TLV/CBOR와 CRC를 쓰는 편이
안전하다. 현재 parser는 escape 문자를 허용하지 않으며 길이와 숫자 범위를 검사한다.

## 5. ESP32 AT 펌웨어 전제

ESP32-C3에는 Espressif ESP-AT 펌웨어가 설치되어 있어야 한다. 애플리케이션은
`AT`, `ATE0`, station mode, AP 접속, TCP 접속 순서로 진행하고 실패 시 1, 2, 4, 8,
16, 30초 exponential backoff로 재연결한다. `WIFI DISCONNECT`, `CLOSED`, `ERROR`도
재접속 조건이다. DHCP=false인 경우 AT 펌웨어 버전에 맞는 고정 IP 명령을
`EspAt_Connect()`에 추가해야 한다(현재 설정에는 gateway/netmask가 없으므로 실제 고정
IP 적용은 불가능하다). 따라서 제품 사양에는 gateway/netmask/DNS도 포함시키는 것을
권장한다.

## 6. Flash 배치와 전원 차단 안전성

W25Q40(512 KiB)의 sector 0/1을 설정 A/B 슬롯으로 예약한다. 각 record는 magic,
schema version, 증가 sequence, payload, CRC32를 가지며 유효 CRC 중 sequence가 큰 쪽을
읽는다. 새 sector를 erase/write/verify한 뒤에만 이전 사본을 지우므로 쓰기 중 전원이
꺼져도 한 사본이 남는다. sector 2 이후는 판정치/로그용으로 별도 wear-leveling을
설계한다. Wi-Fi password를 평문 flash에 저장하므로 양산품은 STM32L5 보안 영역 또는
키 기반 암호화, readout protection 및 로그 마스킹을 적용해야 한다.

## 7. 빌드/검증 순서

1. CubeMX 6.x에서 STM32L562CEUx 프로젝트 생성, 위 설정 후 STM32CubeIDE 코드 생성.
2. `App/`를 source folder로 추가하고 include path에 `App` 추가.
3. `app_port.c` weak board 함수 override 및 `App/app_config.h` handle/timeout 조정.
4. 먼저 USB/RS-485 `GETCFG`, 설정 저장과 재부팅 복원을 시험한다.
5. ESP AT command/응답을 logic analyzer로 확인한 뒤 FPGA simulator의 1 Hz frame을 연결한다.
6. flash erase 도중 전원 차단, AP/서버 재부팅, queue overload, UART noise를 fault-injection
   시험하고 IWDG는 모든 필수 task heartbeat가 정상일 때만 refresh한다.
