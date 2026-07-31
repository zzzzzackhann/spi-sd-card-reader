# SD Card Reader via SPI

A bare-metal program that reads and writes to an SD card, with data transfer verified with a logic analyzer.

## What I built

- Firmware in bare-metal C for initializing, reading and writing an SD card over the SPI
- A system for debugging the firmware with UART
- A PulseView template for debugging the SPI bus with a logic analyzer

## What I learned
- SPI protocol
- SD Card reader initialization sequence over SPI
- How to use a logic analyzer for debugging
