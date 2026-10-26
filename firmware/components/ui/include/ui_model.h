#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    UI_SCREEN_START,
    UI_SCREEN_IAQ,
    UI_SCREEN_TEMP,
    UI_SCREEN_HUM,
    UI_SCREEN_SENSOR_ERROR, /**< Replaces IAQ, TEMP and HUM while the sensor gives no results. */
    UI_SCREEN_BRIGHTNESS,
    UI_SCREEN_SHUTDOWN,
} ui_screen_t;

typedef enum {
    UI_DISPLAY_ON,
    UI_DISPLAY_DIM, /**< Lit at a lower backlight level; otherwise the same as ON. */
    UI_DISPLAY_OFF, /**< Panel asleep, backlight held LOW, LVGL stopped. */
} ui_display_state_t;

typedef enum {
    UI_BATTERY_UNKNOWN,
    UI_BATTERY_LEVEL,
    UI_BATTERY_EXTERNAL_POWER, /**< Supply inferred from voltage; charging is not confirmed. */
} ui_battery_state_t;

typedef struct {
    ui_battery_state_t state;
    uint8_t percent; /**< 0..100; used only in UI_BATTERY_LEVEL. */
} ui_battery_t;

typedef struct {
    bool calibrating; /**< The sensor gives no IAQ it trusts yet; value is not set. */
    uint16_t value;   /**< 0..500 */
} ui_iaq_t;

typedef struct {
    ui_iaq_t iaq;
    int16_t temperature_c;    /**< -40..85 */
    uint8_t humidity_percent; /**< 0..100 */
} ui_measurements_t;

/**
 * Complete screen content, copied from application state without hardware handles.
 * Values must already be rounded and within their documented ranges;
 * measurement pages are shown only with a working sensor.
 */
typedef struct {
    ui_screen_t screen;
    bool usb_notice; /**< USB was just plugged in: the page is covered by a lightning bolt. */

    ui_battery_t battery;
    ui_measurements_t measurements;

    uint8_t brightness_percent; /**< Shown on the brightness screen. */
    bool shutdown_yes_selected; /**< Highlighted answer of the shutdown question. */
} ui_model_t;
