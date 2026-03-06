# pico_platformio_touch_mouse

USB HID Mouse firmware for the Raspberry Pi Pico (RP2040). Reads touch coordinates from an XPT2046 resistive touch controller over SPI and translates them into relative mouse movements reported over USB. Built with PlatformIO using the Arduino-Pico (earlephilhower) core, which bundles TinyUSB for USB HID support.

## Hardware Requirements

- Raspberry Pi Pico (RP2040)
- XPT2046 resistive touch controller (standalone module or panel-integrated)
- USB Micro-B cable
- Jumper wires

### Pin Connections

| Pico GPIO | XPT2046 Pin | Signal | Notes |
|-----------|-------------|--------|-------|
| GPIO 18 | CLK | SPI0 SCK | |
| GPIO 19 | DIN | SPI0 MOSI | |
| GPIO 16 | DOUT | SPI0 MISO | |
| GPIO 17 | CS | SPI0 CS | Active low |
| GPIO 20 | PENIRQ | Touch interrupt | Active low, pull-up enabled |
| GPIO 25 | — | Onboard LED | PWM brightness feedback |
| GPIO 0 | — | UART0 TX | Debug output at 115200 baud |

Connect XPT2046 VCC to Pico 3.3 V (pin 36) and GND to any Pico ground pin.

## Build Environment

### Prerequisites

| Tool | Version | Notes |
|------|---------|-------|
| Python | 3.6 or later | Required for PlatformIO CLI |
| PlatformIO Core | latest | Installed via pip |
| platform-raspberrypi | maxgerhardt fork | Pulled automatically by PlatformIO |
| Arduino-Pico core | earlephilhower | Pulled automatically |

The PlatformIO environment will download the Arduino-Pico (earlephilhower) core and the ARM GCC toolchain automatically on first build. An internet connection is required for the initial setup.

### Installing PlatformIO

```bash
pip install platformio
```

A virtual environment is recommended:

```bash
python3 -m venv .venv
source .venv/bin/activate   # Linux / macOS
.venv\Scripts\activate       # Windows
pip install platformio
```

## Building

From inside the `pico_platformio_touch_mouse/` directory:

```bash
platformio run
```

The compiled firmware will be placed at:
```
.pio/build/raspberry-pi-pico/firmware.uf2
```

## Flashing

### Method 1 — PlatformIO upload (recommended)

1. Hold the BOOTSEL button on the Pico, then plug in the USB cable.
2. Release BOOTSEL. The Pico will mount as a USB mass storage device named `RPI-RP2`.
3. Run:
   ```bash
   platformio run --target upload
   ```
   PlatformIO will copy the UF2 to the drive and the Pico will reboot automatically.

### Method 2 — Manual UF2 copy

1. Enter bootloader mode as described above.
2. Copy the UF2 manually:
   ```bash
   cp .pio/build/raspberry-pi-pico/firmware.uf2 /media/$USER/RPI-RP2/
   ```

## Behaviour

After flashing, the Pico enumerates as a standard USB HID mouse. The host requires no drivers beyond what the OS provides natively.

- On first touch (or after the position has been reset), the cursor jumps to the center of the mapped screen area, then follows the finger proportionally.
- Dragging produces relative movement deltas sent via `Mouse.move(dx, dy)`.
- The position tracking state resets 300 ms after the finger lifts, ensuring the next touch starts cleanly.
- The onboard LED fades in brightness while touch is held and returns to full brightness on release.

## Signal Processing

| Stage | Detail |
|-------|--------|
| Sampling | 2 raw ADC readings per axis per cycle, averaged |
| Jitter threshold | Movement deltas smaller than 10 px (mapped) are discarded |
| Poll interval | 15 ms while touch is active |
| Release debounce | State resets 300 ms after PENIRQ goes high |

## Calibration

Calibration constants are defined in `src/main.cpp`:

```cpp
#define X_MIN   150
#define X_MAX  3784
#define Y_MIN   277
#define Y_MAX  3784
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 480
```

To calibrate for a different panel, enable UART debug output and record the raw ADC readings at each screen corner. Update the four `_MIN` / `_MAX` constants accordingly.

## Debugging

UART0 outputs raw ADC readings in real time at 115200 8N1 on GPIO 0 (TX). Connect a USB-to-UART adapter or use a second Pico as a USB serial bridge.

Example output:
```
X: 842, Y: 1203
X: 845, Y: 1197
```

Any serial terminal will work:
```bash
tio /dev/ttyUSB0 -b 115200
```

## Configuration Reference

| Constant | Default | Effect |
|----------|---------|--------|
| `SCREEN_WIDTH` | 800 | Horizontal mapping range |
| `SCREEN_HEIGHT` | 480 | Vertical mapping range |
| `X_MIN` / `X_MAX` | 150 / 3784 | Raw ADC calibration for X |
| `Y_MIN` / `Y_MAX` | 277 / 3784 | Raw ADC calibration for Y |
| `MOUSE_SCALE` | 1 | Multiplier applied to all movement deltas |

## Troubleshooting

| Symptom | Likely Cause | Resolution |
|---------|-------------|------------|
| Device not enumerated as mouse | Incorrect framework or build | Check `platformio.ini` board and core settings |
| No touch response | SPI wiring error | Verify GPIO 16–19 and GPIO 17 CS connections |
| Cursor jumps randomly | ADC noise or wrong calibration | Verify panel calibration constants via UART output |
| Cursor drifts when idle | Jitter threshold too low | Increase the `abs(dx) >= 10` threshold in `main.cpp` |
| UART outputs nothing | Wrong pin or baud rate | Use GPIO 0 TX only; set terminal to 115200 baud |

## License

MIT License. Refer to the Arduino-Pico core and TinyUSB library licenses for their respective terms.