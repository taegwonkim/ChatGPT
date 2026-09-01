# STM32L562 RTC 24-hour interval software reset example

This repository contains the application-owned portion of an STM32CubeMX / STM32CubeIDE
project for an STM32L562. The RTC is clocked from the 32.768 kHz LSE and its wake-up
timer requests a software reset every 24 hours after initialization.

## Generate and open the project

1. Open `STM32L562_RTC_Reset.ioc` in STM32CubeMX (or directly in STM32CubeIDE).
2. Select the exact STM32L562 package/part used by your board if it differs from
   `STM32L562ZETx`, and verify that the board has a 32.768 kHz crystal connected to
   `PC14/PC15`. If it does not, configure a suitable RTC clock source instead.
3. In **Project Manager**, select **STM32CubeIDE** as the toolchain and generate code.
   When CubeMX asks whether to overwrite files already present, retain the application
   files in `Core/` (or restore them from Git after generation).
4. Connect `PA9` (`USART1_TX`) to the RX input of a 3.3 V USB-to-UART adapter and
   connect the grounds. `PA10` is configured as `USART1_RX` but is not required for the
   reset message. Do not connect an RS-232 voltage-level cable directly to the MCU.
5. Import/open the generated project in STM32CubeIDE, build, flash, and run it. Open a
   serial terminal at **115200 baud, 8 data bits, no parity, 1 stop bit (8-N-1)**.

On every boot, including a power-on boot and an RTC wake-up software reset, USART1
transmits the following line before the 24-hour timer is initialized:

```text
[RESET] STM32L562 has been reset.
```

The `.ioc` enables the RTC wake-up timer and its interrupt. `MX_RTC_Init()` uses the
one-second `ck_spre` clock in 17-bit mode and programs `WakeUpCounter=86399`. Because
the hardware interval is `WakeUpCounter + 1` ticks, the interrupt occurs after 86,400
seconds. The HAL handler invokes `HAL_RTCEx_WakeUpTimerEventCallback()`, which issues
`NVIC_SystemReset()`. After reboot, initialization deactivates the retained timer state
and starts a new 24-hour interval; the reset therefore occurs relative to each boot and
does not depend on calendar time.

## Debugging notes

* A debugger may be configured to halt or disconnect when the MCU resets. This does not
  mean the periodic reset failed; inspect `RCC->CSR` or set a breakpoint in
  `HAL_RTCEx_WakeUpTimerEventCallback()` to confirm it.
* The code records the reset flags in `g_reset_cause` before clearing them. Inspect this
  variable in the debugger to distinguish software resets from power-on resets.
* The 24-hour interval begins when `MX_RTC_Init()` arms the wake-up timer. Firmware
  startup time before that call is not part of the interval.
* Do not perform application work in the RTC callback. It intentionally resets
  immediately and never returns.
