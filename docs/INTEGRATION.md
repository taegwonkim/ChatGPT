# CubeIDE 통합 절차

1. CubeMX로 `.ioc`와 STM32CubeIDE 프로젝트를 생성합니다.
2. 이 저장소의 `Core/Inc/*.h`, `Core/Src/*.c`를 생성 프로젝트에 복사합니다.
3. `main.c`의 모든 `MX_*_Init()` 뒤에 `Surge_AppInit()`을 추가합니다.
4. `app_freertos.c`의 `MX_FREERTOS_Init()` USER CODE에서 `Surge_RtosInit()`을 호출합니다.
5. USART/DMA idle callback에서 받은 구간을 stream buffer로 넘기고 즉시 `HAL_UARTEx_ReceiveToIdle_DMA()`를 재arm합니다. DMA half-transfer interrupt는 설계에 따라 disable합니다.
6. `CDC_Receive_FS()`에서는 USB 데이터를 PC command stream buffer에 복사하고 즉시 return합니다. USB TX는 `USBD_BUSY`일 때 delay 후 재시도하되 timeout을 둡니다.
7. `HAL_GPIO_EXTI_Falling_Callback()`에서 `Surge_FpgaTriggerFromISR()`을 호출합니다.
8. board-specific adapter(`surge_port.c`)의 UART, SPI flash, time, reset 함수를 HAL handle/pin에 연결합니다.

콜백 및 generated file 수정은 반드시 `/* USER CODE BEGIN */` 블록 안에 두어 재생성 시 보존하십시오. 먼저 host parser tests, 이후 loopback, ESP AT, FPGA simulator, 장시간 cable-disconnect 시험 순으로 검증합니다.
