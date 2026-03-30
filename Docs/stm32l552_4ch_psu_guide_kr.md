# STM32L552R + FreeRTOS 4채널 전압/전류 제어 프로젝트 가이드

## 1) 목표 시스템
- MCU: **STM32L552RET6** (STM32CubeMX + STM32CubeIDE)
- RTOS: **FreeRTOS (CMSIS-RTOS v2)**
- DAC: **AD5641** (SPI, 채널별 1개 또는 CS 분리 방식)
- 외부 ADC: **MCP3465R** (SPI)
- 통신: **UART(PC 명령/로그)** + **FDCAN(주기 텔레메트리)**
- 기능:
  - UART 명령으로 각 CH 목표 전압 설정
  - ADC로 실측 전압/전류 읽기
  - 오차 보정 루프(목표값과 실측값 일치 유지)
  - 전류 기반 단락/단선 판정
  - 4CH 전압/전류 상태를 UART + FDCAN으로 주기 송신

---

## 2) STM32CubeMX 세부 설정

## 2.1 Pinout
> 실제 핀은 보드/회로에 맞춰 조정하세요.

- **RCC**: HSE 사용(있으면), 없으면 MSI + PLL
- **SYS**: Debug = Serial Wire
- **USART1**
  - Mode: Asynchronous
  - Baud: 115200 (또는 921600)
  - Word length 8bit, Parity None, Stop 1
  - NVIC: USART1 global interrupt enable
- **SPI1 (AD5641 DAC 용)**
  - Full Duplex Master
  - CPOL/CPHA: AD5641 타이밍에 맞춤(일반적으로 Mode 1 권장, 데이터시트 확인 필수)
  - Prescaler: DAC 타이밍 만족 (예: 8~16)
  - NSS: Software
  - CH별 DAC CS GPIO 4개 출력
- **SPI2 (MCP3465R ADC 용)**
  - Full Duplex Master
  - CPOL/CPHA: MCP3465R 권장 모드 적용
  - CRC 비활성
  - NSS: Software
  - ADC CS GPIO 출력
  - DRDY 핀(GPIO Input + EXTI) 사용 권장
- **FDCAN1**
  - Frame format: FD CAN
  - Bit rate switching(BRS): Enable
  - Nominal/Data bit timing 설정(예: Nominal 500kbps, Data 2Mbps)
  - Tx FIFO Queue enable
  - Rx filter 기본 1개(수신 명령 필요 시)
- **GPIO**
  - DAC CS(4개), ADC CS(1개), 상태 LED, Fault 핀 등
- **TIM6** (선택)
  - 마이크로초 delay 또는 주기 트리거 필요 시

## 2.2 Clock Configuration (예시)
- SYSCLK: 80 MHz
- AHB: 80 MHz
- APB1/APB2: 80 MHz
- FDCAN 커널클럭은 목표 비트레이트에 맞춰 별도 검증

## 2.3 NVIC 우선순위
- USART1 IRQ: 높음 (예: preempt 5)
- EXTI(DRDY): 높음 (예: preempt 6)
- SPI IRQ (DMA 사용 시): 중간
- FDCAN IRQ: 중간
- FreeRTOS 규칙: `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` 보다 높은 IRQ에서 RTOS API 호출 금지

---

## 3) FreeRTOS 설정 (중요)

## 3.1 CubeMX > Middleware > FreeRTOS
- Interface: CMSIS_V2
- Heap scheme: heap_4
- Total heap size: **32KB 이상 권장**
- Tick rate: 1000 Hz
- USE_TIMERS: Enable
- Timer task priority: Normal
- Minimal stack size: 256 words
- Mutex, Queue, EventGroup 활성

## 3.2 태스크 설계

1. **controlTask**
   - 주기: 10ms
   - 우선순위: AboveNormal
   - 역할:
     - CH0~CH3 전압/전류 샘플 갱신
     - 오차 보정 후 DAC 코드 갱신
     - 단락/단선/포화 상태 판정

2. **uartRxTask**
   - 이벤트 기반(Queue block)
   - 우선순위: High
   - 역할:
     - ISR에서 수신 바이트 Queue push
     - 라인 파싱: `SET CHn x.xxx`

3. **telemetryTask**
   - 주기: 100ms
   - 우선순위: Normal
   - 역할:
     - 4CH 스냅샷 UART 문자열 송신
     - FDCAN FD frame 64B로 바이너리 송신

4. **watchdogTask (옵션)**
   - 주기: 50ms
   - 우선순위: BelowNormal
   - 역할:
     - 루프 타임 초과/ADC 응답 누락 감지
     - 오류 플래그와 복구 시퀀스

## 3.3 동기화 객체
- `appDataMutex`: 채널 데이터 보호
- `uartRxQueue`: 수신 바이트 전달 (길이 128)
- 필요 시 `EventGroup`
  - BIT0: ADC_DRDY
  - BIT1: FDCAN_TX_DONE
  - BIT2: FAULT_LATCH

## 3.4 ISR ↔ Task 설계 포인트
- ISR에서는 **최소 처리만 수행**:
  - UART: byte enqueue + 재수신 arm
  - ADC DRDY: 이벤트 set
- 실연산(파싱/보정/송신)은 task context에서 수행

---

## 4) 제어/진단 알고리즘

## 4.1 폐루프 보정
- 기본식:
  - `err = v_set - v_meas`
  - `v_dac_cmd = v_set + Kp * err`
- 권장 시작값: `Kp = 0.2 ~ 0.35`
- 데드밴드: `|err| < 10mV` 면 직접 목표코드 사용
- DAC code 변환:
  - `code = round((v_dac_cmd / Vout_max) * (2^14 - 1))`

## 4.2 전류 기반 이상 판정
- `i_meas >= I_SHORT` → SHORT
- `i_meas <= I_OPEN` → OPEN
- DAC 상/하한 근접 상태는 SAT_HIGH/SAT_LOW
- 상태가 N회 연속 검출 시 latch (예: 3회)

## 4.3 텔레메트리 포맷
- UART(ASCII):
  - `CH0:vset/vmeas/imeas/status,...CH3...`
- FDCAN(FD 64B):
  - CH 구조체 4개를 packed binary 전송
  - 수신 PC 툴에서 동일 struct로 디코딩

---

## 5) 코드 구성 (CubeIDE)

- `Core/Inc/app_config.h`: 상수/임계값/채널 구조체
- `Core/Inc/app_control.h`: 제어 API
- `Core/Src/app_control.c`: 제어 루프, UART 파싱, 텔레메트리
- `Core/Src/freertos_app.c`: 태스크/큐/뮤텍스 생성, ISR 연계

> MCP3465R 및 AD5641의 실제 SPI 레지스터 시퀀스는 보드 회로(채널 mux/샨트 증폭기)에 따라 조정하세요.

---

## 6) UART 명령 예시
- `SET CH0 1.250`
- `SET CH1 3.300`
- `SET CH2 0.500`
- `SET CH3 4.750`

필요 시 확장:
- `GET ALL`
- `SET ALL 2.500`
- `LIM CH1 I 0.120`

---

## 7) 안정화 팁
- ADC 측정에 moving average(4~16샘플) 적용
- 전압 루프와 전류 판정에 서로 다른 필터 상수 적용
- FDCAN 송신 실패시 카운터 누적 + 재시도
- UART는 DMA+IDLE 방식으로 확장하면 고속/저부하에 유리

---

## 8) 빌드/통합 순서
1. CubeMX에서 기본 초기화 코드 생성
2. 본 문서의 `Core/Inc`, `Core/Src` 파일 추가
3. `main.c`에서 `MX_FREERTOS_Init()` 이후 `osKernelStart()` 실행 확인
4. SPI/UART/FDCAN init 파라미터를 실제 하드웨어에 맞게 수정
5. 실측치와 임계값(`CH_SHORT_CURRENT_A`, `CH_OPEN_CURRENT_A`) 캘리브레이션
