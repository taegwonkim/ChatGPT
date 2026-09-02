# STM32L562 periodic-reset examples

이 저장소에는 서로 독립적인 STM32CubeMX/STM32CubeIDE 예제 두 개가 있습니다.

| 예제 | 프로젝트 파일 | 리셋 기준 |
| --- | --- | --- |
| 24시간 간격 방식 | `Examples/IntervalReset/STM32L562_Interval_Reset.ioc` | `MX_RTC_Init()` 실행 후 86,400초 |
| Alarm A 방식 | `Examples/AlarmAReset/STM32L562_AlarmA_Reset.ioc` | RTC 달력 기준 매일 00:00:00 |

두 예제는 코드를 공유하지 않으므로 원하는 디렉터리의 `.ioc` 파일만 열어 각각
별도의 STM32CubeIDE 프로젝트로 생성할 수 있습니다.

## 공통 동작

* MCU는 Sleep/Stop/Standby 모드로 들어가지 않습니다. `while (1)`이 계속 실행되며
  `__WFI()`나 전원 절약 API를 호출하지 않습니다.
* 매 부팅 시 `RCC->CSR`의 리셋 플래그를 읽은 후 USART3로 리셋 종류와 원시 플래그를
  전송합니다. RTC 콜백에서 소프트웨어 리셋되면 다음 부팅에서 `type=SOFTWARE`가
  출력됩니다.
* 메인 루프는 반복할 때마다 `g_loop_count`를 증가시키고, 5초마다 누적 루프 횟수와
  `HAL_GetTick()` 기반 부팅 후 경과 시간을 USART3로 보냅니다.
* USART3 설정은 115200 baud, 8-N-1이며 `PB10=TX`, `PB11=RX`입니다. `PB10`을 3.3 V
  USB-to-UART 어댑터의 RX에 연결하고 GND를 공통으로 연결하십시오. RS-232 레벨을
  MCU 핀에 직접 연결하면 안 됩니다.

출력 예시는 다음과 같습니다.

```text
[BOOT] MCU reset: type=SOFTWARE, RCC_CSR=0x14000000
[LOOP] count=1234567, uptime=5 s
[LOOP] count=2469134, uptime=10 s
```

루프 횟수는 CPU 속도, 컴파일 최적화, 인터럽트 및 UART 전송 시간에 따라 달라집니다.
정확한 시간 측정값이 아니라 펌웨어가 계속 실행 중임을 확인하는 heartbeat 값입니다.

## 1. 24시간 간격 방식

`Examples/IntervalReset`은 RTC Wake-up Timer의 1 Hz `ck_spre`와 17비트 모드를
사용합니다. `WakeUpCounter=86399`이므로 `86399 + 1 = 86400`초 후 리셋합니다.
재부팅할 때 기존 Wake-up Timer 상태를 해제하고 새로운 24시간 주기를 시작하므로
RTC 달력 시각과 관계없이 **부팅할 때마다 24시간 후** 리셋됩니다.

자세한 사용 방법은 `Examples/IntervalReset/README.md`를 참조하십시오.

## 2. Alarm A 방식

`Examples/AlarmAReset`은 Date/Weekday 필드를 마스킹한 Alarm A를 사용하여 RTC 달력
기준 매일 `00:00:00`에 리셋합니다. 기본 Alarm 시각은 `rtc.c`의
`RTC_ALARM_HOUR`, `RTC_ALARM_MINUTE`, `RTC_ALARM_SECOND`에서 변경할 수 있습니다.
실제 제품에서는 최초 부팅 시 RTC를 실제 UTC 또는 현지 시각으로 설정해야 합니다.

자세한 사용 방법은 `Examples/AlarmAReset/README.md`를 참조하십시오.
