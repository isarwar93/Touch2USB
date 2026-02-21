# USB Resistive Touch Controller for Raspberry Pi Pico

This project implements a USB HID digitizer device on the Raspberry Pi Pico microcontroller, converting resistive touch input from an XPT2046 touch controller into absolute touch coordinates. The device appears as a touchscreen input device to the host computer, suitable for applications requiring precise absolute positioning.

## Features

- Absolute touch coordinate reporting
- SPI communication with XPT2046 resistive touch controller
- PWM-controlled LED feedback with dimming effect during touch
- USB HID digitizer protocol implementation
- UART debugging output on GPIO 0/1
- Built using Raspberry Pi Pico SDK and TinyUSB

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