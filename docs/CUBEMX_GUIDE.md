# STM32CubeMX 설정 가이드

## 1. 프로젝트 및 클록

1. MCU selector에서 `STM32L562CET6`를 선택하고 CubeIDE toolchain을 사용합니다.
2. TrustZone은 단일 non-secure 예제가 필요하면 비활성화합니다. 제품 보안 정책상 활성화할 경우 아래 주변장치와 DMA를 Non-Secure에 배정하십시오.
3. HSE가 보드에 있으면 HSE+PLL로 SYSCLK 110 MHz, 없으면 MSI+PLL로 110 MHz를 구성합니다. APB 클록과 USART baud 오차를 Clock Configuration에서 확인합니다.
4. FreeRTOS의 time base와 HAL tick 충돌을 피하려고 `SYS > Timebase Source = TIM6`로 설정합니다. SysTick은 RTOS가 사용합니다.

## 2. 핀/주변장치

| 기능 | CubeMX 설정 |
|---|---|
| USART1 / ESP32 | Asynchronous, 115200-8-N-1, TX/RX, RX DMA circular 권장 |
| USART2 / FPGA | Asynchronous, 921600-8-N-1(실제 RTL과 일치), TX/RX, RX DMA circular |
| USART3 / RS-485 | Asynchronous, 115200-8-N-1. Hardware DE가 배선됐으면 RS485 mode, 아니면 GPIO output으로 DE 제어 |
| USB | USB_OTG_FS Device Only, USB_DEVICE Middleware = CDC |
| SPI2 / W25Q40CL | Full Duplex Master, 8 bit, software NSS, Mode 0, prescaler로 초기 10 MHz 이하. 별도 CS GPIO output(high) |
| FPGA trigger | GPIO_EXTI, Falling edge, no pull(회로에 따라 pull-up), EXTI IRQ 활성화 |
| Watchdog | IWDG 약 4초 권장; 모든 핵심 태스크 heartbeat를 supervisor가 확인한 뒤 refresh |

USART DMA 채널은 RX/TX 모두 활성화하고 DMA interrupt를 enable 합니다. Cortex-M33 cache/DMA 정합성 문제를 피하도록 DMA 버퍼를 정렬하고, cache를 켰다면 clean/invalidate 처리를 추가합니다.

## 3. NVIC 우선순위

FreeRTOS에서 ISR이 `...FromISR()` API를 호출하려면 해당 IRQ priority가 `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`보다 같거나 낮은 논리 우선순위여야 합니다. 예시(숫자가 클수록 낮은 우선순위):

* DMA/USART/EXTI/USB: preemption priority 6
* PendSV/SysTick: CubeMX/FreeRTOS 자동 설정(최저)
* priority grouping: 4 bits preemption

ISR에서는 파싱/flash/network 작업을 하지 말고 byte/frame 또는 notification만 queue/task로 넘깁니다.

## 4. FreeRTOS (CMSIS-RTOS v2) 상세 설정

FreeRTOS middleware를 CMSIS-RTOS v2로 추가하고 다음을 권장합니다.

* Heap: `heap_4`, total heap 최소 32 KiB (실제 high-water mark로 조정)
* `configUSE_PREEMPTION=1`, `configUSE_TIME_SLICING=1`
* tick rate 1000 Hz, max priorities 7
* mutex, recursive mutex, counting semaphore, task notification 활성화
* stack overflow check 2, malloc failed hook 활성화
* software timer task priority 3, queue length 10, stack 512 words

| Task | Priority | Stack(words) | 역할 |
|---|---:|---:|---|
| `Supervisor` | AboveNormal(4) | 512 | 상태/heartbeat/IWDG, FPGA START 1회 |
| `FpgaRx` | High(5) | 768 | USART2 DMA stream 파싱, trigger 후 프레임 수신 |
| `Wifi` | Normal(3) | 1024 | ESP AT 상태기계, 재접속, socket 송신 |
| `PcCommand` | Normal(3) | 768 | USART3/USB CDC line parser, read/write 응답 |
| `Flash` | BelowNormal(2) | 512 | W25Q 설정 load/save 직렬화 |
| `PcTx` | Normal(3) | 512 | RS-485 및 USB 전송(느린 PC가 acquisition을 막지 않음) |

### RTOS 객체

* `fpgaFrameQueue`: 8개 `SurgeFrame` (소유권 명확화를 위해 값 복사 또는 고정 block pool 사용)
* `wifiTxQueue`, `pcTxQueue`: 각 소비자용 별도 queue. 하나의 queue를 두 소비자가 경쟁 소비하면 안 됩니다.
* `configQueue`: 길이 1 overwrite queue 또는 event flag; 저장 완료 후 Wi-Fi task에 reconfigure 알림
* `espMutex`: USART1 AT command/데이터 전송 단일 소유. 본 설계에서는 Wifi task만 USART1을 소유하므로 보통 불필요
* Event flags: `CFG_READY`, `WIFI_UP`, `SERVER_UP`, `FPGA_TRIGGER`, `FLASH_DIRTY`

`Surge_AppInit()`는 모든 peripheral init 뒤, `osKernelStart()` 전에 호출합니다. CubeMX가 만든 `MX_FREERTOS_Init()`에서 `Surge_RtosInit()`을 호출하십시오.

## 5. 데이터 흐름과 복구

부팅 시 flash의 두 설정 슬롯 중 CRC/sequence가 유효한 최신 슬롯을 읽습니다. 없으면 안전한 기본값을 사용합니다. Supervisor는 USART2로 `START\r\n`을 한 번 보내고 FPGA trigger EXTI를 기다립니다. FPGA RX task는 길이/CRC가 유효한 프레임만 타임스탬프와 sequence를 붙여 PC와 Wi-Fi queue 양쪽에 복제합니다.

Wi-Fi task 상태는 `RESET → AT_SYNC → SET_MODE → JOIN_AP → NET_CONFIG → OPEN_SOCKET → ONLINE`입니다. 실패 시 socket close 후 지수 backoff(1, 2, 4, 8, 16, 최대 30초)에 jitter를 더해 재시도합니다. `WIFI DISCONNECT`, `CLOSED`, send timeout 또는 반복 `SEND FAIL`을 받으면 재접속합니다. ONLINE 중에도 주기적으로 `AT`/상태 조회로 연결을 확인합니다.

설정 write는 RAM에서 전체 검증 후 flash의 반대 슬롯에 erase/program/verify하고 마지막에 commit marker를 기록합니다. 성공 응답 이후 Wi-Fi task를 재구성합니다. ISR 또는 acquisition task에서 flash erase를 수행하지 않습니다.
