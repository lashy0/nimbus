#pragma once

typedef enum {
    BUTTONS_PREV,
    BUTTONS_NEXT,
} buttons_id_t;

typedef enum {
    BUTTONS_PRESS_SHORT, /**< Released before the long press time. */
    BUTTONS_PRESS_LONG,  /**< Held for the long press time; once per hold, nothing on release. */
} buttons_press_t;

typedef struct {
    buttons_id_t id;
    buttons_press_t press;
} buttons_event_t;
