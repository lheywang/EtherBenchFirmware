# EtherBenchFirmware

## Overview

- Core firmware for the EtherBench remote hardware debugging and instrumentation platform.
- Target MCU: STM32H5 series (ARM Cortex-M33).
- Role: Hardware orchestration and remote link management (Ethernet / USB).

## Key Features & Architecture

- RTOS Core: Built on Eclipse ThreadX (Azure RTOS) for task management and software stacks (NetXDuo for Ethernet, USBX for USB, LevelX and FileX for the filesystems).
- Semaphore and DMA driven engines : Ensure the smoothest reponse in any situations, as no task is relying on delays to run.

## Communication & Protocols

### Ethernet
Provide multiple services, such as : 
- Telnet server to enter commands
- GPIB raw socket over port 1234 and 5000
- GDB-RSP over port 2159 to get the highest programming performances without requiring a local server.
- FTP server to access to files
- UDP streaming port pairs over port 9000 to provide high bandwidth for usages such as high speed logging

### USB
Provide multiple services, such as : 
- Two USART terminals. One used as a control terminal, same features as the Telnet and the other is used as a raw USB-USART bridge.
- Custom class to enumerate as USB debugger.

## Peripherals Orchestration
- WS2812 aRGB status ring (DMA + Timer single-pulse PWM shaping with gamma correction).
- Onboard I2C display driver.
- Programmable power rail management (TPS buck-boost / PMIC control).
- External QSPI memory interfacing for configuration and state logging as well as an SD slot.

## Hardware Stack
- MCU: STM32H563
- Network: LAN8742A / RMII Ethernet PHY (default on the nucleo, easier to dev with)
- USB: USB2514B
- Power Management: TPS55288 PPS
- Storage: High-speed QSPI 128 MB NAND Flash
- Visual Interface: 20-pixel WS2812 aRGB Ring + I2C OLED/LCD screen

## Build & Toolchain
- Compiler: arm-none-eabi-gcc (C23 / C++23).
- Build System: CMake (minimum version 3.22) + Ninja
- Formatting: LLVM style for C/C++, Black for associated Python test scripts
- Documentation: Doxygen-compliant header comments. Main doc is available on the website associated with the top [repo](https://github.com/lheywang/EtherBench)
