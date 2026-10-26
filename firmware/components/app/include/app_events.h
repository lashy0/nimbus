#pragma once

#include <stdint.h>

typedef enum {
    APP_BUTTON_PREV,
    APP_BUTTON_NEXT,
} app_button_t;

typedef enum {
    APP_GESTURE_PRESSED,
    APP_GESTURE_LONG,  /**< Once per hold. */
    APP_GESTURE_SHORT, /**< Release without LONG. */
    APP_GESTURE_RELEASED,
} app_gesture_t;

/** Button gesture with a monotonic recognition timestamp, copied into the app queue. */
typedef struct {
    app_button_t button;
    app_gesture_t gesture;
    int64_t timestamp_ms; /**< Monotonic time the gesture was recognized, not when app_task took it. */
} app_button_event_t;
