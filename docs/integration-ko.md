# CubeIDE 통합 절차

1. CubeMX에서 `docs/cubemx-setup-ko.md`대로 프로젝트를 생성한다.
2. `App/Inc`를 compiler include path에, `App/Src/*.c`를 build source에 추가한다.
3. `app_port.h`의 함수를 `Core/Src/app_port_stm32.c`에서 HAL, FreeRTOS, USB CDC와
   W25Q40 driver로 구현한다. port 함수는 timeout을 받아 무한정 점유하지 않게 한다.
4. `MX_FREERTOS_Init()`에서 `surge_app_init()` 후 표의 queue/task를 생성한다. scheduler
   시작 전 W25Q40에서 두 config slot을 읽고 가장 높은 valid sequence를 선택한다.
5. `HAL_UARTEx_ReceiveToIdle_DMA()`로 세 UART RX를 시작한다. callback에서 DMA buffer의
   새 범위만 해당 stream buffer에 `xStreamBufferSendFromISR()`로 전달한다.
6. `HAL_GPIO_EXTI_Falling_Callback()`에서 FPGA trigger pin이면 timestamp를 저장하고
   `vTaskNotifyGiveFromISR(fpgaTaskHandle, ...)`를 호출한다.
7. `CDC_Receive_FS()`에서 USB source id와 payload를 PC command message buffer에 넣는다.

## main에서 scheduler 시작

실제 시작 순서는 [`Core/Src/main.c`](../Core/Src/main.c)의 예제처럼 구성한다.

1. `HAL_Init()`과 `SystemClock_Config()`를 먼저 실행한다.
2. GPIO/DMA/SPI/UART/USB를 초기화한다. DMA는 UART보다 먼저 초기화해야 한다.
3. `Surge_AppHardwareStart()`에서 W25Q40 설정을 읽고 UART receive-to-idle DMA를 arm한다.
   이 시점에는 scheduler가 실행 중이 아니므로 `osDelay`, queue blocking wait 등은 금지한다.
4. `osKernelInitialize()`로 CMSIS-RTOS2 kernel을 초기화한다.
5. `MX_FREERTOS_Init()`에서 mutex, queue와 모든 task를 생성한다.
6. `osKernelStart()`를 호출한다. 성공하면 이 함수는 반환하지 않는다.

`Core/Src/freertos.c`의 `stack_size` 단위는 **byte**이다. CubeMX GUI가 word 단위로
표시되는 버전도 있으므로 생성된 `osThreadAttr_t.stack_size` 값과 혼동하지 않는다.
모든 object/thread 생성 결과를 확인하여 heap 부족을 scheduler 시작 전에 검출한다.

## W25Q40 이중 slot

4 KiB sector 두 개를 A/B slot으로 사용한다. header는 magic, schema version, payload
length, monotonically increasing sequence, payload CRC32를 갖는다. 새 설정은 inactive
sector erase → header/payload program → read-back CRC verify 순으로 쓴다. 마지막 valid
slot은 새 slot 검증이 끝날 때까지 지우지 않는다. 전원 차단 후에도 둘 중 하나가 남는다.

W25Q40의 JEDEC ID를 boot 때 확인하고 write-enable latch, BUSY timeout, page(256-byte)
경계를 driver에서 처리한다. 설정이 자주 바뀌는 환경에서는 erase 횟수를 기록하거나
wear-leveling을 추가한다.

## 필수 callback 원칙

ISR/callback은 byte 복사, timestamp, semaphore/notification만 수행한다. AT 응답 parsing,
CRC 계산, flash erase, USB transmit는 task context에서 실행한다. FreeRTOS API를 호출하는
모든 IRQ priority가 `configMAX_SYSCALL_INTERRUPT_PRIORITY` 규칙을 지키는지 생성 후
`FreeRTOSConfig.h`와 NVIC를 다시 비교한다.

## 실장 검증 순서

1. flash JEDEC ID와 A/B slot 전원 차단 복구
2. USB와 RS-485 각각 SET/GET/SAVE, 동시 명령 경쟁
3. FPGA START가 reset당 한 번인지, trigger/frame/CRC timeout
4. AP 부재 및 TCP server 중단 시 30초 상한 backoff와 task stack high-water mark
5. 6채널 최대 baud 지속 입력에서 frame loss, pool exhaustion, watchdog
6. USB cable reconnect, RS-485 DE timing, ESP reset 중에도 FPGA acquisition 지속 여부
