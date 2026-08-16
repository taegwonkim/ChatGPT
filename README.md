# SurgeDetector firmware

STM32L562CET6 + FreeRTOS 기반 SurgeDetector의 CubeMX 설정 지침과 CubeIDE에 바로
추가할 수 있는 애플리케이션 계층 예제이다. CubeMX가 생성하는 HAL/USB 파일은 MCU
패키지 버전에 종속되므로 저장소에는 생성 파일 대신 사용자 코드와 통합 절차를 둔다.

## 문서

* [`docs/cubemx-setup-ko.md`](docs/cubemx-setup-ko.md): 핀, DMA, NVIC, USB CDC와
  FreeRTOS 객체/우선순위/스택의 상세 설정
* [`docs/protocol-ko.md`](docs/protocol-ko.md): PC 설정 명령, FPGA 프레임, 서버 전송 형식
* [`docs/integration-ko.md`](docs/integration-ko.md): CubeIDE 프로젝트에 코드 추가 및 HAL
  callback 연결 방법

## 소스 구성

`App/`는 CubeMX 재생성의 영향을 받지 않는 애플리케이션 계층이다. `app_config`와
`pc_protocol`은 HAL 비의존이라 PC에서도 테스트할 수 있고, 나머지 모듈은 포트
인터페이스(`app_port.h`)만 호출한다. 실제 HAL/FreeRTOS 호출은 프로젝트별
`app_port_stm32.c`에서 구현한다.

`Core/Src/main.c`에는 peripheral 초기화 후 CMSIS-RTOS2 kernel을 시작하는 순서가,
`Core/Src/freertos.c`에는 RTOS object와 여섯 task를 생성하는 코드가 들어 있다. CubeMX
재생성 시 해당 내용을 `USER CODE` 구역에 유지하고 생성된 clock/peripheral init body와
결합한다.

```text
App/Inc/       public headers
App/Src/       protocol, configuration and task state machines
tests/         host-side unit tests
docs/          CubeMX and wire protocol specification
```

## 빠른 검증

```bash
make test
```

실장 시험에서는 반드시 RS-485 종단/DE 극성, ESP32 AT firmware의 명령 집합,
FPGA 프레임 endian/길이, W25Q40 erase 수명을 최종 하드웨어와 함께 확인한다.
