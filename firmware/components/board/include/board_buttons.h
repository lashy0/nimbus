#pragma once

#include <stdint.h>

#include "esp_err.h"

typedef enum {
    BOARD_BUTTON_PREV,
    BOARD_BUTTON_NEXT,
    BOARD_BUTTON_COUNT,
} board_button_t;

typedef enum {
    BOARD_BUTTON_PRESSED,  // Debounced press, including the press that woke the device.
    BOARD_BUTTON_LONG,     // 1500 ms after the press, once per hold.
    BOARD_BUTTON_SHORT,    // Release without LONG; reported before RELEASED.
    BOARD_BUTTON_RELEASED, // Debounced release.
} board_button_event_type_t;

typedef struct {
    board_button_t button;
    board_button_event_type_t type;
    int64_t timestamp_us; // esp_timer time when the debounced state was observed.
} board_button_event_t;

// Runs in the esp_timer task: copy the event and return without blocking.
typedef void (*board_button_sink_fn)(void* ctx, const board_button_event_t* event);

// Call after board_init_safe_gpio(). Debounce 50 ms; holds ESP_PM_NO_LIGHT_SLEEP while
// any button is pressed. A press already held here yields no SHORT/LONG until released.
// Call once. On error, buttons created before the failure keep reporting to the sink.
esp_err_t board_buttons_init(board_button_sink_fn sink, void* ctx);
