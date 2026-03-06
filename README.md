# RP2040 XPT2046 Resistive Touch to USB HID

Firmware for the Raspberry Pi Pico (RP2040) that bridges an XPT2046 SPI resistive touch controller to a USB HID device. Two independent firmware implementations are provided, targeting different use cases.

## Repository Structure

```
Touch2USB/
├── pico_platformio_touch_mouse/   # USB HID Mouse via PlatformIO / Arduino framework
└── pico_sdk_touch/               # USB HID Digitizer via Pico SDK + TinyUSB
```

| Firmware | Framework | USB Class | Coordinate Mode | Host Compatibility |
|---|---|---|---|---|
| `pico_platformio_touch_mouse` | PlatformIO / Arduino | HID Mouse | Relative delta | Any OS with generic HID mouse |
| `pico_sdk_touch` | Pico SDK + TinyUSB | HID Digitizer | Absolute (touchscreen) | Linux evdev / LVGL, Windows, macOS |

## Hardware

### Bill of Materials

- Raspberry Pi Pico (RP2040)
- XPT2046 resistive touch controller module (or a display panel with an integrated XPT2046)
- USB Micro-B cable
- Jumper wires

### Wiring

Both firmware implementations use the same physical connections.

| Pico GPIO | XPT2046 Pin | Signal | Notes |
|-----------|-------------|--------|-------|
| GPIO 18 | CLK / DCLK | SPI0 SCK | |
| GPIO 19 | DIN | SPI0 MOSI | |
| GPIO 16 | DOUT | SPI0 MISO | |
| GPIO 17 | CS / CSB | SPI0 CS | Active low, driven by firmware |
| GPIO 20 | PENIRQ | Touch interrupt | Active low, internal pull-up enabled |
| GPIO 25 | — | Onboard LED | PWM feedback, optional |
| GPIO 0 | — | UART0 TX | Debug output at 115200 baud, optional |
| GPIO 1 | — | UART0 RX | Not used for output; complete the pair |

> The XPT2046 VCC pin accepts 2.7 V to 5.5 V. Connect to the Pico's 3.3 V output (pin 36). Connect GND to any Pico ground pin.

### SPI Configuration

- Controller: SPI0
- Clock speed: 1 MHz
- Mode: CPOL=0, CPHA=0 (Mode 0)
- Word order: MSB first
- ADC resolution: 12 bits

## Touch Calibration

The XPT2046 returns 12-bit ADC values that must be mapped to screen coordinates. The following calibration constants were measured on a 4-wire resistive panel and are used by both firmware targets:

| Axis | Raw ADC Minimum | Raw ADC Maximum | Screen Range |
|------|-----------------|-----------------|-------------|
| X | 150 | 3784 | 0 – 799 |
| Y | 277 | 3784 | 0 – 479 |

If your panel returns different ADC limits, update `X_MIN`, `X_MAX`, `Y_MIN`, and `Y_MAX` in the respective source file. To determine correct values, enable UART debug output and record the raw ADC readings at each screen corner.

## Signal Processing

Resistive touch panels are electrically noisy. Both implementations include noise rejection, details of which are documented in each firmware's own README. The Pico SDK digitizer implementation uses a more complete pipeline:

1. **5-sample median filter** — eliminates single-sample electrical spikes before any further processing.
2. **Exponential moving average (EMA)** — low-pass filters the mapped coordinate stream to smooth cursor motion while preserving responsiveness.
3. **Jitter suppression threshold** — suppresses HID reports when the smoothed position has not moved meaningfully, preventing cursor trembling during a stationary hold.
4. **Release guard timer** — debounces the PENIRQ line on lift-off to eliminate spurious pen-up events mid-drag.

## Firmware Selection Guide

Use `pico_platformio_touch_mouse` when:
- The host application expects a standard USB mouse.
- Relative movement is acceptable or preferred.
- You want the simplest possible build environment (PlatformIO handles the toolchain).

Use `pico_sdk_touch` when:
- The host runs LVGL via the evdev or libinput backend and needs a proper touchscreen input device.
- Absolute coordinate reporting is required (kiosk, embedded HMI, drawing tablet).
- You need full control over the USB HID descriptor.

## Detailed Documentation

- [pico_platformio_touch_mouse/README.md](pico_platformio_touch_mouse/README.md)
- [pico_sdk_touch/README.md](pico_sdk_touch/README.md)

## License

MIT License. See individual SDK and library licenses (Pico SDK, TinyUSB, Arduino-Pico core) for their respective terms.