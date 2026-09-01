# STM32L562 RTC daily software reset example

This repository contains the application-owned portion of an STM32CubeMX / STM32CubeIDE
project for an STM32L562. The RTC wake-up timer is clocked from the 32.768 kHz LSE and
requests a software reset every 24 hours (86,400 seconds).

## Generate and open the project

1. Open `STM32L562_RTC_Reset.ioc` in STM32CubeMX (or directly in STM32CubeIDE).
2. Select the exact STM32L562 package/part used by your board if it differs from
   `STM32L562ZETx`, and verify that the board has a 32.768 kHz crystal connected to
   `PC14/PC15`. If it does not, configure a suitable RTC clock source instead.
3. In **Project Manager**, select **STM32CubeIDE** as the toolchain and generate code.
   When CubeMX asks whether to overwrite files already present, retain the application
   files in `Core/` (or restore them from Git after generation).
4. Import/open the generated project in STM32CubeIDE, build, flash, and run it.

The `.ioc` enables RTC wake-up and its interrupt. `MX_RTC_Init()` selects the 17-bit
wake-up counter and programs it for 86,399 + 1 one-second RTC `ck_spre` ticks. The
17-bit mode is necessary because a 24-hour count does not fit in the 16-bit mode. The
interrupt is handled by the HAL,
which invokes `HAL_RTCEx_WakeUpTimerEventCallback()`; the callback issues
`NVIC_SystemReset()`.

## Debugging notes

* A debugger may be configured to halt or disconnect when the MCU resets. This does not
  mean the periodic reset failed; inspect `RCC->CSR` or set a breakpoint in
  `HAL_RTCEx_WakeUpTimerEventCallback()` to confirm it.
* The code records the reset flags in `g_reset_cause` before clearing them. Inspect this
  variable in the debugger to distinguish software resets from power-on resets.
* The first reset occurs approximately 24 hours after `MX_RTC_Init()` runs. Every
  reboot re-arms the timer, so boot time is not included in the 24-hour interval.
* Do not perform application work in the RTC callback. It intentionally resets
  immediately and never returns.
