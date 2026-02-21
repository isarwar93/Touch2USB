#define CFG_TUD_HID 1

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/pwm.h"
#include "hardware/uart.h"
#include "tusb.h"
#include "device/usbd.h"
#include "class/hid/hid_device.h"
#include "usb_descriptors.h"

// SPI pins
#define SPI_PORT spi0
#define PIN_SCK 18
#define PIN_MOSI 19
#define PIN_MISO 16
#define PIN_CS 17
#define PIN_INTR 20
#define LED_PIN 25

// Touch controller commands for XPT2046
#define CMD_READ_X 0xD0
#define CMD_READ_Y 0x90

// Screen resolution
// #define SCREEN_WIDTH 32767
// #define SCREEN_HEIGHT 32767

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480

// Touch raw value ranges
#define X_MIN 150
#define X_MAX 3784
#define Y_MIN 277
#define Y_MAX 3784
#define X_RANGE (X_MAX - X_MIN)
#define Y_RANGE (Y_MAX - Y_MIN)

// #define SCREEN_WIDTH 1200
// #define SCREEN_HEIGHT 720

static uint16_t read_touch_axis(uint8_t cmd) {
    uint8_t tx_buf[3] = {cmd, 0, 0};
    uint8_t rx_buf[3];

    gpio_put(PIN_CS, 0);
    spi_write_read_blocking(SPI_PORT, tx_buf, rx_buf, 3);
    gpio_put(PIN_CS, 1);

    uint16_t result = ((rx_buf[1] << 8) | rx_buf[2]) >> 3; // 12-bit
    return result;
}

int main() {
    stdio_init_all();

    // Initialize UART for debugging
    uart_init(uart0, 115200);
    gpio_set_function(0, GPIO_FUNC_UART); // TX
    gpio_set_function(1, GPIO_FUNC_UART); // RX

    // Initialize SPI
    spi_init(SPI_PORT, 1000000);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    // CS pin
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    // Interrupt pin
    gpio_init(PIN_INTR);
    gpio_set_dir(PIN_INTR, GPIO_IN);
    gpio_pull_up(PIN_INTR);

    // LED pin with PWM
    gpio_set_function(LED_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(LED_PIN);
    pwm_set_wrap(slice, 255);
    pwm_set_enabled(slice, true);
    pwm_set_gpio_level(LED_PIN, 255); // Full brightness

    // Initialize TinyUSB
    tusb_init();

    uint64_t touch_start = 0;
    bool touching = false;

    while (true) {
        tud_task();

        if (gpio_get(PIN_INTR) == 0) { // Touch detected
            if (!touching) {
                touching = true;
                touch_start = time_us_64() / 1000;
                pwm_set_gpio_level(LED_PIN, 0); // Full brightness
                // uint16_t x_raw = read_touch_axis(CMD_READ_X);
                // uint16_t y_raw = read_touch_axis(CMD_READ_Y);
                // uint16_t x = (x_raw * SCREEN_WIDTH) / 4096;
                // uint16_t y = (y_raw * SCREEN_HEIGHT) / 4096;
                // printf("Touch: X=%u, Y=%u\n", x, y);
            } else {
                uint64_t elapsed = (time_us_64() / 1000) - touch_start;
                int brightness = 255 - (elapsed / 50); // Dim over ~12.75 seconds
                if (brightness < 0) brightness = 0;
                pwm_set_gpio_level(LED_PIN, brightness);
            }

            uint16_t x_raw = 0, y_raw = 0;
            for (int i = 0; i < 2; i++) {
                x_raw += read_touch_axis(CMD_READ_X);
                y_raw += read_touch_axis(CMD_READ_Y);
            }
            x_raw /= 2;
            y_raw /= 2;

            // Map to absolute coordinates with calibration
            uint16_t x = ((x_raw - X_MIN) * SCREEN_WIDTH) / X_RANGE;
            uint16_t y = ((y_raw - Y_MIN) * SCREEN_HEIGHT) / Y_RANGE;

            // Clamp to screen bounds
            if (x > SCREEN_WIDTH - 1) x = SCREEN_WIDTH - 1;
            if (y > SCREEN_HEIGHT - 1) y = SCREEN_HEIGHT - 1;
            printf("Raw: X=%u, Y=%u | Mapped: X=%u, Y=%u\n", x_raw, y_raw, x, y);
            // Send digitizer HID report (8 bytes total with report ID)
            uint8_t report[8];
            report[0] = 0x01; // Report ID
            report[1] = 0x03; // Tip Switch (bit 0) and In Range (bit 1) both set
            report[2] = x & 0xFF;
            report[3] = x >> 8;
            report[4] = y & 0xFF;
            report[5] = y >> 8;
            // report[6] = 0x80; // Pressure (128 = medium pressure when touching)
            report[6] = 0xff; // Pressure (128 = medium pressure when touching)
            report[7] = 0x01; // Contact count = 1

            if (tud_hid_n_ready(0)) {
                tud_hid_n_report(0, 0, report, 8);  // Report ID 0 since it's included in data
            }

            sleep_ms(10); // Debounce
        } else {
            if (touching) {
                touching = false;
                pwm_set_gpio_level(LED_PIN, 255); // Back to full brightness
                // Send no touch report
                uint8_t report[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Report ID, tip=0, in_range=0, contact_count=0
                tud_hid_n_report(0, 0, report, 8);
            }
        }
    }

    return 0;
}