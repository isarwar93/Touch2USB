#include "config.h"

#include <Mouse.h>

// SPI pins
#define PIN_CS 17
#define PIN_INTR 20
#define LED_PIN 25

// Touch controller commands for XPT2046
#define CMD_READ_X 0xD0
#define CMD_READ_Y 0x90

// Screen resolution (adjust as needed)
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480

// Touch raw value ranges
#define X_MIN 150
#define X_MAX 3784
#define Y_MIN 277
#define Y_MAX 3784
#define X_RANGE (X_MAX - X_MIN)
#define Y_RANGE (Y_MAX - Y_MIN)

// Mouse sensitivity
#define MOUSE_SCALE 1

static uint16_t read_touch_axis(uint8_t cmd) {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(PIN_CS, LOW);
    SPI.transfer(cmd);
    uint8_t high = SPI.transfer(0);
    uint8_t low = SPI.transfer(0);
    digitalWrite(PIN_CS, HIGH);
    SPI.endTransaction();

    uint16_t result = ((high << 8) | low) >> 3; // 12-bit
    return result;
}

void setup() {
    // Initialize SPI
    SPI.begin();

    // CS pin
    pinMode(PIN_CS, OUTPUT);
    digitalWrite(PIN_CS, HIGH);

    // Interrupt pin
    pinMode(PIN_INTR, INPUT_PULLUP); // Assuming active low

    // LED pin
    pinMode(LED_PIN, OUTPUT);
    analogWrite(LED_PIN, 255); // Turn on full brightness

    // Initialize Mouse
    Mouse.begin();

    // Initialize UART0 for debugging
    Serial1.begin(115200);
}

void loop() {
    static int16_t last_x = -1, last_y = -1;
    static unsigned long touch_start = 0;
    static bool touching = false;
    static unsigned long last_touch_time = 0;

    if (digitalRead(PIN_INTR) == LOW) { // Touch detected
        if (!touching) {
            touching = true;
            touch_start = millis();
            analogWrite(LED_PIN, 0); // Turn off LED on touch start
        } else {
            unsigned long elapsed = millis() - touch_start;
            int brightness = 255 - (elapsed / 10); 
            if (brightness < 0) brightness = 0;
            analogWrite(LED_PIN, brightness);
        }

        // Average 2 readings to reduce noise
        uint16_t x_raw = 0, y_raw = 0;
        for (int i = 0; i < 2; i++) {
            x_raw += read_touch_axis(CMD_READ_X);
            y_raw += read_touch_axis(CMD_READ_Y);
        }
        x_raw /= 2;
        y_raw /= 2;

        // Print raw values over UART
        Serial1.print("X: ");
        Serial1.print(x_raw);
        Serial1.print(", Y: ");
        Serial1.println(y_raw);

        // Map raw values to screen coordinates with offsets
        int16_t x = ((x_raw - X_MIN) * SCREEN_WIDTH) / X_RANGE;
        int16_t y = ((y_raw - Y_MIN) * SCREEN_HEIGHT) / Y_RANGE;

        // Clamp to screen bounds
        x = constrain(x, 0, SCREEN_WIDTH - 1);
        y = constrain(y, 0, SCREEN_HEIGHT - 1);

        if (last_x != -1 && last_y != -1) {
            int8_t dx = (x - last_x) * MOUSE_SCALE;
            int8_t dy = (y - last_y) * MOUSE_SCALE;

            // Only move if change is significant to avoid jitter
            if (abs(dx) >= 10 || abs(dy) >= 10) {
                Mouse.move(dx, dy);
                last_x = x;
                last_y = y;
            }
        } else {
            // Initialize last position to center of screen for absolute-like jump on first touch
            last_x = SCREEN_WIDTH / 2;
            last_y = SCREEN_HEIGHT / 2;
        }

        delay(15); // Debounce
    } else {
        if (touching) {
            touching = false;
            last_touch_time = millis();
            analogWrite(LED_PIN, 255); // Back to full brightness when touch ends
        }
        // Reset position if no touch for 300 milliseconds
        if (millis() - last_touch_time > 300) {
            last_x = -1;
            last_y = -1;
        }
    }
}