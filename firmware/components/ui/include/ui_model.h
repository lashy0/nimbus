#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    UI_SCREEN_START,
    UI_SCREEN_IAQ,
    UI_SCREEN_TEMP,
    UI_SCREEN_HUM,
    UI_SCREEN_SENSOR_ERROR, // Replaces IAQ, TEMP and HUM while the sensor gives no results.
    UI_SCREEN_BRIGHTNESS,
    UI_SCREEN_SHUTDOWN,
} ui_screen_t;

typedef enum {
    UI_DISPLAY_ON,
    UI_DISPLAY_DIM, // Lit at a lower backlight level; otherwise the same as ON.
    UI_DISPLAY_OFF, // Panel asleep, backlight held LOW, LVGL stopped.
} ui_display_state_t;

// Everything the screens show. Values are already rounded and within their ranges;
// the measurement pages are shown only with a working sensor.
typedef struct {
    ui_screen_t screen;
    bool usb_notice; // USB was just plugged in: the page is covered by a lightning bolt.

    bool external_power; // Supply inferred from the voltage; battery_valid is then false.
    bool battery_valid;
    uint8_t battery_percent; // 0..100
    bool charging;

    bool iaq_calibrating; // The sensor gives no IAQ it trusts yet; iaq is not set.
    uint16_t iaq;         // 0..500

    int16_t temperature_c;    // -40..85
    uint8_t humidity_percent; // 0..100

    uint8_t brightness_percent; // Shown on the brightness screen.
    bool shutdown_yes_selected; // Highlighted answer of the shutdown question.
} ui_model_t;
