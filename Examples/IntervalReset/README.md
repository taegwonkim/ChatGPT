# RTC Wake-up Timer 24-hour interval reset

1. `STM32L562_Interval_Reset.ioc`를 STM32CubeMX 또는 STM32CubeIDE에서 엽니다.
2. 보드가 예제와 다른 패키지라면 정확한 STM32L562 부품으로 변경합니다.
3. 보드의 `PC14/PC15`에 32.768 kHz LSE 크리스털이 있는지 확인합니다.
4. STM32CubeIDE Toolchain으로 코드를 생성하되 이 디렉터리의 `Core` 애플리케이션
   파일은 유지합니다.
5. `PB10`을 3.3 V USB-to-UART RX에 연결하고 터미널을 115200, 8-N-1로 엽니다.

RTC Wake-up Counter는 86,399이며 1초 tick을 86,400번 센 뒤 콜백에서
`NVIC_SystemReset()`을 실행합니다. 리셋 후 USART3 부팅 메시지가 출력되고 새로운
24시간 주기가 시작됩니다. 메인 루프는 저전력 모드에 진입하지 않고 계속 실행되며
5초마다 heartbeat를 출력합니다.

