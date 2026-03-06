# pico_sdk_touch

USB HID Digitizer firmware for the Raspberry Pi Pico (RP2040). Converts XPT2046 resistive touch input into absolute USB touch coordinates using the Pico SDK and TinyUSB. The device enumerates as a single-touch HID digitizer (touchscreen), compatible with the Linux evdev/libinput stack, LVGL, Windows, and macOS — no drivers required.

## Hardware Requirements

- Raspberry Pi Pico (RP2040)
- XPT2046 resistive touch controller (standalone module or panel-integrated)
- USB Micro-B cable
- Jumper wires

### Pin Connections

| Pico GPIO | XPT2046 Pin | Signal | Notes |
|-----------|-------------|--------|-------|
| GPIO 18 | CLK / DCLK | SPI0 SCK | |
| GPIO 19 | DIN | SPI0 MOSI | |
| GPIO 16 | DOUT | SPI0 MISO | |
| GPIO 17 | CS / CSB | SPI0 CS | Active low, firmware-controlled |
| GPIO 20 | PENIRQ | Touch interrupt | Active low, internal pull-up enabled |
| GPIO 25 | — | Onboard LED | PWM brightness feedback |
| GPIO 0 | — | UART0 TX | Debug output at 115200 baud |
| GPIO 1 | — | UART0 RX | Not actively used |

Connect XPT2046 VCC to Pico 3.3 V (pin 36) and GND to any Pico ground pin.

## Build Environment

### System Dependencies

| Tool | Minimum Version | Notes |
|------|-----------------|-------|
| CMake | 3.13 | Build system |
| GNU Arm Embedded Toolchain | 10.3-2021.10 or later | `gcc-arm-none-eabi` |
| Make | any | Or Ninja — both work |
| Raspberry Pi Pico SDK | 1.5.1 or later | Must include TinyUSB submodule |

On Ubuntu / Debian:

```bash
sudo apt update
sudo apt install cmake gcc-arm-none-eabi build-essential git
```

### Pico SDK Setup

Clone the SDK and initialise its submodules (TinyUSB is one of them):

```bash
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init
```

Export the path so CMake can find it:

```bash
export PICO_SDK_PATH=/path/to/pico-sdk
```

Add that line to `~/.bashrc` or `~/.profile` to make it permanent.

## Project Structure

```
pico_sdk_touch/
├── CMakeLists.txt        # Build configuration
├── main.c                # Application logic, signal processing, HID reporting
├── usb_descriptors.h     # Extern declarations
├── usb_descriptors.c     # HID report descriptor, device/config/string descriptors,
│                         # TinyUSB descriptor callbacks
└── build/                # Generated — not committed to source control
    ├── pico_touch.uf2    # Flash image (drag-and-drop to RPI-RP2 drive)
    ├── pico_touch.elf    # ELF with debug symbols
    └── pico_touch.bin    # Raw binary
```

## Building

```bash
cd pico_sdk_touch
mkdir -p build && cd build
cmake .. -DPICO_SDK_PATH="$PICO_SDK_PATH"
make -j$(nproc)
```

If `PICO_SDK_PATH` is already in your environment, the `-D` flag is optional.

Successful output ends with:
```
[100%] Built target pico_touch
```

## Flashing

### Method 1 — UF2 drag-and-drop (no tools required)

1. Hold BOOTSEL on the Pico and plug in USB.
2. The Pico mounts as a mass storage device named `RPI-RP2`.
3. Copy the firmware:
   ```bash
   cp build/pico_touch.uf2 /media/$USER/RPI-RP2/
   ```
   The Pico unmounts and reboots automatically.

### Method 2 — picotool (device must already be running Pico SDK firmware)

```bash
picotool load build/pico_touch.elf --force-no-reboot
picotool reboot
```

## Behaviour

After flashing, the Pico enumerates as a USB HID digitizer. The host OS registers it as an absolute-position touchscreen input device with no additional driver or udev configuration required.

- Touch events are reported as absolute pixel coordinates in the range X: 0–799, Y: 0–479.
- The onboard LED dims gradually while a finger is held on the panel and resets to full brightness on lift.
- UART0 prints raw and smoothed coordinate pairs in real time for calibration and debugging.

## Signal Processing Pipeline

Resistive touch panels exhibit significant electrical noise. The following stages run in order on each polling cycle:

| Stage | Implementation | Purpose |
|-------|---------------|---------|
| 1. Median filter | 5 raw ADC samples per axis; return the median | Eliminates single-sample electrical spikes unconditionally |
| 2. Calibration mapping | `(raw - MIN) * SCREEN_DIM / RANGE` | Maps 12-bit ADC space to pixel coordinates |
| 3. Clamping | Clipped to `[0, SCREEN_DIM - 1]` | Guards against out-of-range calibration inputs |
| 4. EMA low-pass filter | α = 3/8 (integer arithmetic) | Smooths coordinate stream; seeded on first sample per touch |
| 5. Jitter suppression | Discard report if Δx < 12 px AND Δy < 12 px | Prevents cursor trembling during stationary hold |
| 6. Release guard | Confirm lift only after PENIRQ high for 40 ms | Eliminates spurious pen-up events during slow drags |

All tuning constants are `#define`d at the top of `main.c`.

## Calibration

Calibration constants in `main.c`:

```c
#define X_MIN   150    // Raw ADC at left edge
#define X_MAX  3784    // Raw ADC at right edge
#define Y_MIN   277    // Raw ADC at top edge
#define Y_MAX  3784    // Raw ADC at bottom edge
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 480
```

To calibrate a different panel, enable UART debug output and touch each of the four corners in turn. Record the raw ADC values printed and update the four constants.

## HID Descriptor Details

The device presents a single-touch HID Digitizer (Usage Page 0x0D, Usage 0x04). The HID report descriptor declares axis logical maximums of 799 and 479, exactly matching the values sent by firmware. This is important: the Linux kernel stores the declared logical maximum in `input_absinfo.maximum`; if the declared range does not match the transmitted range, evdev-based stacks (libinput, LVGL evdev backend) will mis-scale coordinates.

### Report Format (Report ID 0x01, 8 bytes total)

| Byte | Field | Value |
|------|-------|-------|
| 0 | Report ID | 0x01 |
| 1 | Tip Switch [bit 0], In Range [bit 1], padding [bits 2–7] | 0x03 = touching, 0x00 = released |
| 2–3 | X coordinate (little-endian uint16) | 0 – 799 |
| 4–5 | Y coordinate (little-endian uint16) | 0 – 479 |
| 6 | Tip pressure (uint8) | 0xFF while touching |
| 7 | Contact count (uint8) | 1 while touching, 0 on release |

## Debugging

UART0 outputs one line per sent HID report at 115200 8N1 on GPIO 0 (TX):

```
Raw: X=1842 Y=2103 | Smooth: X=421 Y=237
```

Connect a USB-to-UART adapter to GPIO 0 and monitor with any serial terminal:

```bash
tio /dev/ttyUSB0 -b 115200
```

To verify the device is correctly enumerated on Linux:

```bash
# List HID devices
lsusb | grep -i touch

# Show input device axes and properties
evtest /dev/input/event<N>

# Inspect kernel-registered axis ranges
udevadm info /dev/input/event<N>
```

## Configuration Reference

| Constant | Default | Effect |
|----------|---------|--------|
| `SCREEN_WIDTH` | 800 | Horizontal pixel range reported to host |
| `SCREEN_HEIGHT` | 480 | Vertical pixel range reported to host |
| `X_MIN` / `X_MAX` | 150 / 3784 | Raw 12-bit ADC calibration bounds for X |
| `Y_MIN` / `Y_MAX` | 277 / 3784 | Raw 12-bit ADC calibration bounds for Y |
| `NUM_SAMPLES` | 5 | Median filter sample count per axis per cycle |
| `EMA_ALPHA_NUM/DEN` | 3/8 | EMA smoothing factor (lower = smoother, more lag) |
| `JITTER_THRESHOLD` | 12 px | Minimum movement to trigger a HID report |
| `POLL_MS` | 8 ms | Polling interval while touch is active |
| `RELEASE_GUARD_MS` | 40 ms | PENIRQ debounce window on lift-off |

## Troubleshooting

| Symptom | Likely Cause | Resolution |
|---------|-------------|------------|
| `PICO_SDK_PATH not set` at cmake | Environment variable missing | `export PICO_SDK_PATH=/path/to/pico-sdk` |
| CMake cannot find `pico_sdk_init.cmake` | SDK submodules not initialised | `cd pico-sdk && git submodule update --init` |
| Device not enumerated | USB wiring or TinyUSB misconfiguration | Check `lsusb`; verify `CFG_TUSB_RHPORT0_MODE` is set |
| Coordinates stuck top-left on host | Axis range mismatch in HID descriptor | Ensure logical maximum in descriptor matches `SCREEN_WIDTH/HEIGHT` |
| No touch response | SPI wiring error | Verify GPIO 16–19 continuity; check PENIRQ pull-up |
| Jittery cursor during hold | Jitter threshold too low | Increase `JITTER_THRESHOLD` in `main.c` |
| Spurious lift events mid-drag | Release guard too short | Increase `RELEASE_GUARD_MS` in `main.c` |

## License

MIT License. Refer to the Raspberry Pi Pico SDK and TinyUSB licenses for their respective terms.

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

## Software Requirements

### Raspberry Pi Pico SDK

1. Install the Raspberry Pi Pico SDK:
   ```bash
   git clone https://github.com/raspberrypi/pico-sdk.git
   cd pico-sdk
   git submodule update --init
   ```

2. Set the PICO_SDK_PATH environment variable:
   ```bash
   export PICO_SDK_PATH=/path/to/your/pico-sdk
   ```

   Add this to your shell profile (e.g., ~/.bashrc) for persistence.

### Build Tools

- CMake (version 3.13 or later)
- GNU Arm Embedded Toolchain
- Make

On Ubuntu/Debian:
```bash
sudo apt update
sudo apt install cmake gcc-arm-none-eabi build-essential
```

## Project Structure

```
pico_touch_sdk/
├── CMakeLists.txt          # Build configuration
├── main.c                  # Main application code
├── usb_descriptors.h       # USB descriptor declarations
├── usb_descriptors.c       # USB descriptor definitions and callbacks
├── build/                  # Build output directory (generated)
│   ├── pico_touch.uf2     # Firmware file for flashing
│   ├── pico_touch.elf     # Executable with debug symbols
│   └── pico_touch.bin     # Binary image
└── README.md              # This file
```

## Building the Project

1. Navigate to the project directory:
   ```bash
   cd /path/to/pico_touch_sdk
   ```

2. Create a build directory:
   ```bash
   mkdir build
   cd build
   ```

3. Configure the build with CMake:
   ```bash
   cmake ..
   ```

4. Build the project:
   ```bash
   make -j4
   ```

The build process will generate the following files in the `build/` directory:
- `pico_touch.uf2`: The firmware file for flashing to the Pico
- `pico_touch.elf`: The executable with debug symbols
- `pico_touch.bin`: The binary image

## Flashing to Raspberry Pi Pico

1. Put the Raspberry Pi Pico into bootloader mode:
   - Press and hold the BOOTSEL button while plugging in the USB cable
   - Or, if the Pico is already powered, press and hold BOOTSEL while pressing the RESET button

2. The Pico will appear as a USB drive named "RPI-RP2"

3. Copy the UF2 file to the drive:
   ```bash
   cp pico_touch.uf2 /media/$USER/RPI-RP2/
   ```

4. The Pico will automatically reboot with the new firmware

## Usage

1. Connect the XPT2046 touch controller according to the pin connections table
2. Power the Pico via USB
3. The device will enumerate as a USB HID digitizer
4. Touch events will be reported as absolute coordinates
5. The LED will dim during touch and reset to full brightness when touch ends
6. UART debug output is available on GPIO 0 (TX) at 115200 baud

### Touch Coordinate Mapping

- Raw 12-bit ADC values from XPT2046 are calibrated and mapped to screen coordinates
- Touch readings are averaged over 2 samples to reduce noise
- Coordinates are clamped to screen bounds

### Calibration

The touch screen has been calibrated with the following raw value ranges:
- X-axis: 150 to 3784
- Y-axis: 277 to 3784

These values are used to map touch coordinates accurately to the screen resolution. If your touch screen has different ranges, update `X_MIN`, `X_MAX`, `Y_MIN`, and `Y_MAX` in `main.c`.
- X and Y coordinates are reported in the HID report
- Touch detection uses the PENIRQ pin for interrupt-driven reading

### HID Report Format

The device sends HID reports with the following structure:
- Report ID: 1
- Tip Switch: 1 when touching, 0 when not
- In Range: 1 when touching, 0 when not
- X Coordinate: 16-bit absolute value
- Y Coordinate: 16-bit absolute value
- Contact Count: Number of touch points (1 or 0)

## Configuration

### Touch Parameters

- Screen resolution: Configurable (default 800x480 pixels, mapped to 16-bit HID coordinates)
- SPI speed: 1 MHz
- Touch polling: Continuous while PENIRQ is low

### LED Configuration

- PWM frequency: Based on system clock
- Dimming rate: Decreases brightness over approximately 12.75 seconds
- Full brightness reset: On touch release

## Troubleshooting

### Build Issues

1. **PICO_SDK_PATH not set**: Ensure the environment variable points to the correct Pico SDK directory
2. **CMake errors**: Verify CMake version and dependencies are installed
3. **Compilation failures**: Check for missing headers or incorrect include paths

### Runtime Issues

1. **Device not recognized**: Verify USB connection and check device manager for HID digitizer
2. **No touch response**: Check SPI connections and XPT2046 wiring
3. **LED not working**: Confirm GPIO 25 connection and PWM configuration

### Debugging

- Use `picotool` to inspect the running device:
  ```bash
  picotool info -a
  ```

- Monitor USB traffic with tools like Wireshark or usbmon

## License

This project is provided as-is for educational and development purposes. Refer to the Raspberry Pi Pico SDK and TinyUSB licenses for usage terms.

## Contributing

Contributions are welcome. Please ensure code follows the existing style and includes appropriate documentation.