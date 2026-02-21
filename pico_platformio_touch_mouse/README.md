# USB Resistive Touch Controller for Raspberry Pi Pico

This project implements a USB HID mouse controller using the Raspberry Pi Pico (RP2040) microcontroller. It reads touch coordinates from a resistive touch screen via SPI and translates them into relative mouse movements sent over USB using TinyUSB.

## Hardware Requirements

- Raspberry Pi Pico (RP2040)
- Resistive touch screen controller (e.g., XPT2046)
- Connections:
  - GP19: SPI MOSI (Touch TX)
  - GP18: SPI SCK (Touch SCK)
  - GP16: SPI MISO (Touch RX)
  - GP17: SPI CS (Touch CS)
  - GP20: Touch interrupt pin
  - GP0: UART TX (for debugging, optional)
  - GP1: UART RX (for debugging, optional)
  - GP25: LED pin (onboard LED)

## Software Setup

### Prerequisites

- Python 3.6 or later
- Git (optional, for cloning)

### Installation

1. Clone or download this repository.

2. Navigate to the project directory:
   ```
   cd pico_touch_controller
   ```

3. Create a Python virtual environment:
   ```
   python3 -m venv .venv
   ```

4. Activate the virtual environment:
   - On Linux/Mac:
     ```
     source .venv/bin/activate
     ```
   - On Windows:
     ```
     .venv\Scripts\activate
     ```

5. Install PlatformIO:
   ```
   pip install platformio
   ```

## Building the Project

1. Ensure the virtual environment is activated.

2. Build the firmware:
   ```
   platformio run
   ```

## Uploading to Pico

1. Put the Pico into bootloader mode (hold BOOTSEL while plugging in).

2. Upload the firmware:
   ```
   platformio run --target upload
   ```

## Usage

After uploading, the Pico will appear as a USB HID mouse device. Touching the screen will move the cursor to that position (jumping from center on first touch or after reset), and dragging will provide relative mouse movement. The position resets after 1 second of no touch.

### Configuration

- Screen resolution is set to 800x480 pixels. Adjust `SCREEN_WIDTH` and `SCREEN_HEIGHT` in `src/main.cpp` if needed.
- Mouse sensitivity can be adjusted via `MOUSE_SCALE`.
- Touch controller commands are for XPT2046. Modify if using a different controller.
- Touch readings are averaged over 2 samples to reduce noise.
- Movement threshold is set to 2 pixels to reduce jitter from noise.
- On first touch or after reset, the cursor jumps from the center of the screen to the touch position for touchscreen-like behavior.
- Position resets after 1 second of no touch activity.

### Calibration

The touch screen has been calibrated with the following raw value ranges:
- X-axis: 150 to 3784
- Y-axis: 277 to 3784

These values are used to map touch coordinates accurately to the screen resolution. If your touch screen has different ranges, update `X_MIN`, `X_MAX`, `Y_MIN`, and `Y_MAX` in `src/main.cpp`.

### Debugging

Raw touch values are printed over UART0 (GP0 TX, GP1 RX) at 115200 baud for debugging purposes. Connect a serial monitor to these pins to view real-time touch data in the format "X: <value>, Y: <value>".

## Assumptions

- The touch controller is XPT2046-compatible with SPI interface.
- Touch interrupt is active low.
- Raw touch values range from 150-3784 for X and 277-3784 for Y (12-bit ADC with offsets).
- UART debugging is enabled on UART0 (GP0/GP1) at 115200 baud.

## Troubleshooting

- Ensure all SPI pins are correctly connected.
- Verify the touch controller model and adjust commands if necessary.
- Check USB connection and device recognition on the host system.
- For debugging, connect a serial terminal to GP0 (TX) and GP1 (RX) at 115200 baud to monitor raw touch values.

## License

This project is provided as-is for educational and development purposes.