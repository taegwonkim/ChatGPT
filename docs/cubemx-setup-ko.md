# STM32CubeMX 설정 (STM32L562CET6)

## 1. 기본 프로젝트

1. MCU selector에서 **STM32L562CET6**를 선택하고 Toolchain/IDE를 STM32CubeIDE로
   지정한다. TrustZone을 사용하지 않는 단일 이미지 예제라면 `System Core > GTZC`에서
   peripherals를 Non-Secure로 구성한다. 제품 보안 정책상 TrustZone이 필요하면 USB,
   DMA, GPIO와 각 USART의 secure attribution을 별도로 검토해야 한다.
2. `SYS > Debug = Serial Wire`, timebase는 `TIM6`로 설정한다. FreeRTOS에서 HAL tick과
   RTOS tick이 SysTick을 동시에 사용하지 않게 하기 위함이다.
3. HSE가 실제 보드에 있으면 해당 주파수를 입력하고 PLL을 통해 SYSCLK 110 MHz 이하로
   구성한다. USB는 정확한 48 MHz clock을 선택한다. Clock Configuration의 오류가 0인지
   확인한다.
4. I-cache를 enable하고 watchdog이 필요한 양산품은 IWDG를 enable한다. 각 task가
   heartbeat를 갱신하고 supervisor task만 watchdog을 refresh하도록 한다.

## 2. 권장 핀 배치

실제 PCB net와 충돌하면 alternate function이 가능한 다른 핀으로 옮긴다.

| 기능 | peripheral / 예시 핀 | 설정 |
|---|---|---|
| ESP32 | USART1 TX PA9, RX PA10 | 115200, 8-N-1, DMA RX/TX |
| FPGA | USART2 TX PA2, RX PA3 | 921600(협의 가능), 8-N-1, DMA RX/TX |
| PC RS-485 | USART3 TX PB10, RX PB11 | 115200, 8-N-1, DMA RX/TX |
| RS-485 DE | PB1 GPIO output | idle low, 송신 동안 high |
| W25Q40 | SPI2 SCK PB13, MISO PB14, MOSI PB15 | master, mode 0, prescaler로 20 MHz 이하 |
| Flash CS | PB12 GPIO output | idle high |
| FPGA trigger | PC13 GPIO EXTI | falling edge, no-pull(보드 회로에 따라 pull-up) |
| USB FS | PA11 DM, PA12 DP | USB Device FS, CDC |

USART DMA는 **circular RX**로 시작하고 UART idle-line event에서 새 byte 범위를 task에
알린다. DMA buffer는 512 byte(ESP), 2048 byte(FPGA), 512 byte(PC)를 권장한다. DMA TX
완료 ISR에서는 semaphore만 release하며 parsing이나 printf를 하지 않는다.

## 3. NVIC

`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY=5`(4 priority bits 기준)일 때 RTOS API를
호출하는 interrupt의 preemption priority는 수치상 **5 이상**이어야 한다.

| interrupt | preemption priority | ISR 작업 |
|---|---:|---|
| FPGA trigger EXTI | 5 | timestamp + task notification |
| USART1/2/3, 관련 DMA | 6 | byte 위치 계산, stream/message buffer notify |
| USB FS | 7 | CDC middleware 처리 |
| TIM6 HAL tick | 15 | HAL tick |

Subpriority는 모두 0으로 둔다. ISR에서 `...FromISR()` 호출 뒤
`portYIELD_FROM_ISR(higherPriorityTaskWoken)`를 실행한다.

## 4. FreeRTOS (CMSIS-RTOS v2)

FreeRTOS는 native API 사용을 권장한다(`USE_NEWLIB_REENTRANT=0`, 동적 할당 실패 hook,
stack overflow hook mode 2 활성화). Tick은 1 kHz, heap은 우선 48 KiB로 시작하고
실측 high-water mark로 조정한다.

| task | priority | stack (32-bit words) | 역할 / blocking point |
|---|---:|---:|---|
| `FpgaRxTask` | AboveNormal(4) | 768 | trigger 후 USART2 frame 조립; stream buffer 대기 |
| `DataRouterTask` | High(5) | 768 | frame ref를 USB/RS485/Wi-Fi queue로 fan-out; queue 대기 |
| `WifiTask` | Normal(3) | 1024 | ESP AT 상태 머신, 재접속 backoff, TCP 전송; event/queue 대기 |
| `PcCommandTask` | Normal(3) | 768 | USB/USART3 line parser, 설정 변경; message buffer 대기 |
| `FlashTask` | Low(1) | 512 | W25Q40 read/write/verify 직렬화; queue 대기 |
| `SupervisorTask` | Low(1) | 384 | 통계, heartbeat, watchdog; delay-until |

필요 객체:

* `fpgaRxStream` 4096 bytes, `espRxStream` 2048 bytes, `pcRxMessage` 1024 bytes.
* `wifiTxQueue`, `usbTxQueue`, `rs485TxQueue`: 각 `FrameRef` 16개. 큰 ADC payload를
  queue에 복사하지 않고 고정 block pool(예: 24 × 512 bytes)의 reference count를 쓴다.
* `flashRequestQueue`: 4개, `configMutex`: priority inheritance mutex,
  `uart{1,2,3}TxDone`: binary semaphore.
* event group `systemEvents`: `CONFIG_VALID`, `WIFI_ASSOCIATED`, `SERVER_CONNECTED`,
  `FPGA_STARTED`, `USB_READY` bit.

CubeMX의 default task는 삭제하거나 `SupervisorTask`로 바꾼다. 모든 task는 무한 poll
대신 queue/semaphore/notification에 block해야 한다. `printf`는 task 간 원자성을
보장하지 않으므로 production logging queue를 별도로 둔다.

## 5. USB Device

Middleware에서 USB Device/CDC를 선택하고 FS max packet 64 byte를 사용한다.
`CDC_Receive_FS()`는 수신 내용을 message/stream buffer로 복사한 뒤 즉시 다음 receive
packet을 arm한다. `CDC_Transmit_FS()`가 `USBD_BUSY`이면 버리지 말고 USB TX task가
완료 callback/짧은 block 후 재시도한다. ISR/callback에서 flash write나 command parsing을
하지 않는다.

## 6. 메모리와 cache

STM32L5 DMA 접근 가능 SRAM에 DMA buffer를 배치하고 32-byte align한다. 사용하는 device
revision/cache 설정에 따라 DMA 전후 cache clean/invalidate가 필요한지 reference manual로
확인한다. Frame pool이 고갈되면 가장 오래된 telemetry를 drop하고 `dropped_frames`를
증가시키되 FPGA RX task는 막지 않는다.
