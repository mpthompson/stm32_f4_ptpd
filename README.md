# STM32 F4 PTPD

IEEE 1588 PTP daemon for STM32 F4 Discovery board

# Important Note

While this code did work at one time, it is fairly old and runs on STM32 F4 development
boards that are harder to come by.

Please see the updated project [stm32_ptpd](https://github.com/mpthompson/stm32_ptpd "stm32_ptpd") which
includes the following enhancements:

 - Provides an example of a PTPD master and slave
 - Projects for ST NUCLEO-F429ZI dev board which is more widely available
 - Ported PTPD v2 code has been improved and has better comments
 - Supports the STM32 F4 HAL library rather than the older standard library
 - Project files for Keil uVision and Linux friendly Makefile
 - Improved telnet and serial shell for testing
 - Easier network configuration in main.c
