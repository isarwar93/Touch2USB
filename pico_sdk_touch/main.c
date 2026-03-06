#define CFG_TUD_HID 1

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/pwm.h"
#include "hardware/uart.h"
#include "tusb.h"
#include "device/usbd.h"
#include "class/hid/hid_device.h"
#include "usb_descriptors.h"

// ── Hardware pin assignments ───────────────────────────────────────────────
#define SPI_PORT spi0
#define PIN_SCK  18
#define PIN_MOSI 19
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_INTR 20
#define LED_PIN  25

// ── XPT2046 commands ──────────────────────────────────────────────────────
#define CMD_READ_X 0xD0
#define CMD_READ_Y 0x90

// ── Screen resolution ─────────────────────────────────────────────────────
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 480

// ── Touch calibration (raw ADC limits, measured empirically) ──────────────
#define X_MIN   150
#define X_MAX  3784
#define Y_MIN   277
#define Y_MAX  3784
#define X_RANGE (X_MAX - X_MIN)
#define Y_RANGE (Y_MAX - Y_MIN)

// ── Noise-reduction tuning ────────────────────────────────────────────────
//
// MEDIAN FILTER
//   Take NUM_SAMPLES raw readings per axis each cycle, sort them, and pick
//   the middle value.  This rejects single-sample electrical spikes that
//   cannot be eliminated by simple averaging.
#define NUM_SAMPLES 5

// EXPONENTIAL MOVING AVERAGE (EMA / low-pass filter)
//   filtered = alpha * new_sample + (1 - alpha) * previous_filtered
//   alpha = EMA_ALPHA_NUM / EMA_ALPHA_DEN  →  3/8 = 0.375
//   Lower alpha  = smoother but more lag.   Higher = more responsive.
#define EMA_ALPHA_NUM 3
#define EMA_ALPHA_DEN 8

// JITTER SUPPRESSION
//   Only send a HID report when the smoothed coordinate has moved at least
//   this many pixels.  Prevents the cursor trembling while the finger is
//   held still.
#define JITTER_THRESHOLD 12

// POLLING INTERVAL (ms) while a touch is active.
#define POLL_MS 8

// RELEASE GUARD (ms)
//   The interrupt line can briefly float high during a slow drag.  We only
//   confirm "pen up" once it has been continuously high for this long.
//   Eliminates spurious lift events in the middle of a drag.
#define RELEASE_GUARD_MS 40

// ─────────────────────────────────────────────────────────────────────────

// Insertion-sort for small arrays (NUM_SAMPLES is only 5)
static void sort_u16(uint16_t *arr, int n) {
    for (int i = 1; i < n; i++) {
        uint16_t key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j] > key) { arr[j + 1] = arr[j]; j--; }
        arr[j + 1] = key;
    }
}

// Single 12-bit SPI read from XPT2046
static uint16_t read_touch_axis_raw(uint8_t cmd) {
    uint8_t tx_buf[3] = {cmd, 0, 0};
    uint8_t rx_buf[3];
    gpio_put(PIN_CS, 0);
    spi_write_read_blocking(SPI_PORT, tx_buf, rx_buf, 3);
    gpio_put(PIN_CS, 1);
    return ((rx_buf[1] << 8) | rx_buf[2]) >> 3; // 12-bit result
}

// Median-filtered read: take NUM_SAMPLES, return the middle value
static uint16_t read_touch_axis(uint8_t cmd) {
    uint16_t samples[NUM_SAMPLES];
    for (int i = 0; i < NUM_SAMPLES; i++) {
        samples[i] = read_touch_axis_raw(cmd);
    }
    sort_u16(samples, NUM_SAMPLES);
    return samples[NUM_SAMPLES / 2];
}

// EMA low-pass filter (integer arithmetic, no float needed).
// *acc stores the running filtered value as an integer.
// Pass reset=true on the first sample after touch-down to seed the filter.
static uint16_t ema_filter(uint16_t new_val, int32_t *acc, bool reset) {
    if (reset) {
        *acc = (int32_t)new_val;
    } else {
        // new = (alpha_num * new_val + (den - alpha_num) * old) / den
        *acc = (EMA_ALPHA_NUM * (int32_t)new_val
                + (EMA_ALPHA_DEN - EMA_ALPHA_NUM) * (*acc))
               / EMA_ALPHA_DEN;
    }
    return (uint16_t)*acc;
}

int main(void) {
    stdio_init_all();

    // UART debug output
    uart_init(uart0, 115200);
    gpio_set_function(0, GPIO_FUNC_UART); // TX
    gpio_set_function(1, GPIO_FUNC_UART); // RX

    // SPI
    spi_init(SPI_PORT, 1000000);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    // CS pin
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    // Interrupt / pen-down line (active-low, pull-up)
    gpio_init(PIN_INTR);
    gpio_set_dir(PIN_INTR, GPIO_IN);
    gpio_pull_up(PIN_INTR);

    // LED on PWM
    gpio_set_function(LED_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(LED_PIN);
    pwm_set_wrap(slice, 255);
    pwm_set_enabled(slice, true);
    pwm_set_gpio_level(LED_PIN, 255); // Full brightness at idle

    // TinyUSB
    tusb_init();

    bool     touching     = false;
    uint64_t touch_start  = 0;
    uint64_t release_time = 0;   // timestamp of first "pen-up" detection
    int32_t  last_x = -1, last_y = -1;
    int32_t  ema_x  =  0, ema_y  =  0;
    bool     ema_init = false;

    while (true) {
        tud_task();

        bool pen_down = (gpio_get(PIN_INTR) == 0);

        if (pen_down) {
            release_time = 0; // cancel any pending release guard

            if (!touching) {
                touching   = true;
                touch_start = time_us_64() / 1000;
                ema_init   = false;
                last_x = last_y = -1;
                pwm_set_gpio_level(LED_PIN, 0);
            } else {
                // Dim LED gradually while held (mirrors pico_platformio behaviour)
                uint64_t elapsed = (time_us_64() / 1000) - touch_start;
                int brightness = 255 - (int)(elapsed / 10);
                if (brightness < 0) brightness = 0;
                pwm_set_gpio_level(LED_PIN, (uint16_t)brightness);
            }

            // Step 1 – Median-filtered raw read
            uint16_t x_raw = read_touch_axis(CMD_READ_X);
            uint16_t y_raw = read_touch_axis(CMD_READ_Y);

            // Step 2 – Map to screen coordinates using calibration constants
            int32_t x_mapped = ((int32_t)(x_raw - X_MIN) * SCREEN_WIDTH)  / X_RANGE;
            int32_t y_mapped = ((int32_t)(y_raw - Y_MIN) * SCREEN_HEIGHT) / Y_RANGE;

            // Clamp to screen bounds
            if (x_mapped < 0)                x_mapped = 0;
            if (x_mapped > SCREEN_WIDTH  - 1) x_mapped = SCREEN_WIDTH  - 1;
            if (y_mapped < 0)                y_mapped = 0;
            if (y_mapped > SCREEN_HEIGHT - 1) y_mapped = SCREEN_HEIGHT - 1;

            // Step 3 – EMA smoothing (seed on first sample after touch-down)
            uint16_t x_s = ema_filter((uint16_t)x_mapped, &ema_x, !ema_init);
            uint16_t y_s = ema_filter((uint16_t)y_mapped, &ema_y, !ema_init);
            ema_init = true;

            // Step 4 – Jitter suppression: only report if coordinate moved enough
            bool moved = (last_x < 0) ||
                         (abs((int32_t)x_s - last_x) >= JITTER_THRESHOLD) ||
                         (abs((int32_t)y_s - last_y) >= JITTER_THRESHOLD);

            if (moved) {
                last_x = x_s;
                last_y = y_s;

                printf("Raw: X=%u Y=%u | Smooth: X=%u Y=%u\n",
                       x_raw, y_raw, x_s, y_s);

                // Send absolute digitizer HID report
                uint8_t report[8];
                report[0] = 0x01;      // Report ID
                report[1] = 0x03;      // Tip Switch | In Range
                report[2] = x_s & 0xFF;
                report[3] = x_s >> 8;
                report[4] = y_s & 0xFF;
                report[5] = y_s >> 8;
                report[6] = 0xFF;      // Pressure (full)
                report[7] = 0x01;      // Contact count = 1

                if (tud_hid_n_ready(0)) {
                    tud_hid_n_report(0, 0, report, 8);
                }
            }

            sleep_ms(POLL_MS);

        } else {
            if (touching) {
                // Record first moment the pen left the surface
                if (release_time == 0) {
                    release_time = time_us_64() / 1000;
                }

                // Confirm release only after the guard period expires
                if ((time_us_64() / 1000) - release_time >= RELEASE_GUARD_MS) {
                    touching  = false;
                    ema_init  = false;
                    last_x = last_y = -1;
                    release_time = 0;
                    pwm_set_gpio_level(LED_PIN, 255); // Back to full brightness

                    // Send pen-up report
                    uint8_t report[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                    if (tud_hid_n_ready(0)) {
                        tud_hid_n_report(0, 0, report, 8);
                    }
                }
            }
        }
    }

    return 0;
}