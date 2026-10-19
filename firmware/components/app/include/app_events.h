#pragma once

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
