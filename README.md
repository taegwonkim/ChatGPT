# SurgeDetector firmware reference

STM32L562CET6 + FreeRTOS/CMSIS-RTOS2 기반의 SurgeDetector 애플리케이션 예제입니다. 이 저장소는 **CubeMX가 생성하는 HAL/시작 코드 위에 추가할 사용자 코드**와 CubeMX 설정 지침을 제공합니다.

## 문서

* [CubeMX 설정 및 태스크 설계](docs/CUBEMX_GUIDE.md)
* [PC 명령 및 FPGA 프레임 프로토콜](docs/PROTOCOL.md)
* [통합 순서](docs/INTEGRATION.md)

## 디렉터리

* `Core/Inc`, `Core/Src`: 애플리케이션 모듈. CubeIDE 프로젝트의 같은 경로에 복사합니다.
* `tests`: 하드웨어 독립 프로토콜 파서 테스트입니다.

## 중요 하드웨어 확인 사항

USART1은 ESP32 전용, USART2는 FPGA 전용, USART3은 RS-485 PC 통신용입니다. 질문에 기재된 “FPGA 데이터를 MCU(USART1)로부터”는 핀 충돌을 피하기 위해 **FPGA→USART2→MCU→USART1→ESP32** 흐름으로 해석했습니다. ESP32-C-WROOM에는 AT 펌웨어가 미리 설치되어 있어야 합니다.

실제품에서는 ADC 데이터 길이/엔디언, FPGA 트리거 핀, RS-485 DE 핀, ESP32 reset/enable 핀을 회로와 FPGA RTL에 맞게 조정해야 합니다.
