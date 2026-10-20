#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    UI_SCREEN_START,
    UI_SCREEN_IAQ,
    UI_SCREEN_TEMP,
    UI_SCREEN_HUM,
    UI_SCREEN_BRIGHTNESS,
    UI_SCREEN_SHUTDOWN,
} ui_screen_t;

typedef enum {
    UI_DISPLAY_ON,
    UI_DISPLAY_DIM, // Lit at a lower backlight level; otherwise the same as ON.
    UI_DISPLAY_OFF, // Panel asleep, backlight held LOW, LVGL stopped.
} ui_display_state_t;

typedef enum {
    UI_IAQ_NONE,        // No reading yet.
    UI_IAQ_CALIBRATING, // The sensor reports a value it does not trust yet.
    UI_IAQ_VALID,
} ui_iaq_state_t;

// Everything the screens show. Values are already rounded and within their ranges.
typedef struct {
    ui_screen_t screen;
    bool usb_notice; // USB was just plugged in: the page is covered by a lightning bolt.

    bool external_power; // Supply inferred from the voltage; battery_valid is then false.
    bool battery_valid;
    uint8_t battery_percent; // 0..100
    bool charging;

    ui_iaq_state_t iaq_state;
    uint16_t iaq; // 0..500

    bool temperature_valid;
    int16_t temperature_c; // -40..85

    bool humidity_valid;
    uint8_t humidity_percent; // 0..100

    uint8_t brightness_percent; // Shown on the brightness screen.
    bool shutdown_yes_selected; // Highlighted answer of the shutdown question.
} ui_model_t;
