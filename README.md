# STM32L562 RTC daily software reset example

This repository contains the application-owned portion of an STM32CubeMX / STM32CubeIDE
project for an STM32L562. The RTC calendar is clocked from the 32.768 kHz LSE and RTC
Alarm A requests a software reset every day at 00:00:00.

## Generate and open the project

1. Open `STM32L562_RTC_Reset.ioc` in STM32CubeMX (or directly in STM32CubeIDE).
2. Select the exact STM32L562 package/part used by your board if it differs from
   `STM32L562ZETx`, and verify that the board has a 32.768 kHz crystal connected to
   `PC14/PC15`. If it does not, configure a suitable RTC clock source instead.
3. In **Project Manager**, select **STM32CubeIDE** as the toolchain and generate code.
   When CubeMX asks whether to overwrite files already present, retain the application
   files in `Core/` (or restore them from Git after generation).
4. Import/open the generated project in STM32CubeIDE, build, flash, and run it.

The `.ioc` enables RTC Alarm A and its interrupt. `MX_RTC_Init()` masks the alarm's
date/weekday field, so Alarm A matches once per day at 00:00:00. The HAL interrupt
handler invokes `HAL_RTC_AlarmAEventCallback()`, which issues `NVIC_SystemReset()`.
Change `RTC_ALARM_HOUR`, `RTC_ALARM_MINUTE`, and `RTC_ALARM_SECOND` in
`Core/Src/rtc.c` to select another daily reset time.

On the first boot only, the example initializes the calendar to 2024-01-01 00:00:01
and writes a magic value to backup register DR0. A real product should set the RTC from
its actual UTC/local time source. Later software resets preserve the calendar. The code
also waits until the alarm-matching second has passed before re-arming Alarm A, avoiding
an immediate reset loop after the MCU reboots at midnight.

## Debugging notes

* A debugger may be configured to halt or disconnect when the MCU resets. This does not
  mean the periodic reset failed; inspect `RCC->CSR` or set a breakpoint in
  `HAL_RTC_AlarmAEventCallback()` to confirm it.
* The code records the reset flags in `g_reset_cause` before clearing them. Inspect this
  variable in the debugger to distinguish software resets from power-on resets.
* Alarm A resets the MCU at the configured time of day, rather than 24 hours after
  `MX_RTC_Init()`. With the included first-boot calendar value, the first reset occurs
  just under 24 hours later.
* Do not perform application work in the RTC callback. It intentionally resets
  immediately and never returns.
