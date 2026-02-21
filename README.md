# RP2040 SPI Resistive Touch → USB HID Interface

This repository contains firmware for the Raspberry Pi Pico (RP2040) that converts an SPI-based resistive touch controller into a USB Human Interface Device (HID).

## Project Structure

The project is organized into two firmware implementations:

- **pico_platformio_touch_mouse/**: Converts touch input into a USB Mouse device using PlatformIO.
- **pico_sdk_touch/**: Converts touch input into a USB Touch (Digitizer) device using Pico SDK.

## Features

- SPI communication with resistive touch controller (e.g., XPT2046)
- USB HID device implementation
- Mouse emulation firmware (relative movement)
- Touch digitizer firmware (absolute coordinates)
- PWM-controlled LED feedback with dimming effect during touch
- UART debugging output on GPIO 0/1
- Designed for RP2040 (Pico SDK / TinyUSB compatible)

## Hardware Requirements

- Raspberry Pi Pico (RP2040 microcontroller)
- XPT2046 resistive touch controller
- Connecting wires
- USB cable for programming and power
- Optional: LED connected to GPIO 25 for visual feedback

### Pin Connections

| Pico GPIO | XPT2046 Pin | Function          |
|-----------|-------------|-------------------|
| 18        | SCK         | SPI Clock         |
| 19        | MOSI        | SPI Master Out    |
| 16        | MISO        | SPI Master In     |
| 17        | CS          | Chip Select       |
| 20        | PENIRQ      | Touch Interrupt   |
| 25        | -           | LED (PWM output)  |
| 0         | -           | UART TX (debug)   |
| 1         | -           | UART RX (debug)   |

## Use Cases

- Custom USB touch panels
- DIY USB touch displays
- Embedded UI devices
- Industrial / kiosk input systems
- Learning USB HID implementation on RP2040

## Getting Started

Each subfolder contains its own README with detailed setup, building, and flashing instructions:

- [pico_platformio_touch_mouse/README.md](pico_platformio_touch_mouse/README.md) - PlatformIO-based mouse implementation
- [pico_sdk_touch/README.md](pico_sdk_touch/README.md) - Pico SDK-based digitizer implementation

## Configuration

- Touch controller: XPT2046-compatible
- SPI speed: 1 MHz
- Touch calibration ranges: X (150-3784), Y (277-3784)
- Screen resolution: Configurable (default 800x480 for mouse, 32767x32767 logical max for digitizer)