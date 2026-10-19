#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    APP_BUTTON_PREV,
    APP_BUTTON_NEXT,
} app_button_t;

typedef enum {
    APP_GESTURE_PRESSED,
    APP_GESTURE_LONG,  // Once per hold.
    APP_GESTURE_SHORT, // Release without LONG.
    APP_GESTURE_RELEASED,
} app_gesture_t;

typedef struct {
    app_button_t button;
    app_gesture_t gesture;
} app_button_event_t;

// Latest measurements as their sources report them; app rounds and clamps for display.
typedef struct {
    bool iaq_valid;
    uint8_t iaq_accuracy; // BSEC accuracy 0..3; 0 means the sensor is still calibrating.
    float iaq;

    bool temperature_valid;
    float temperature_c;

    bool humidity_valid;
    float humidity_percent;

    bool battery_valid;
    uint8_t battery_percent;
    bool charging;
} app_readings_t;
