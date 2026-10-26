#pragma once

#include <stdint.h>

#include "esp_err.h"

typedef enum {
    BOARD_BUTTON_PREV,
    BOARD_BUTTON_NEXT,
    BOARD_BUTTON_COUNT,
} board_button_t;

typedef enum {
    BOARD_BUTTON_PRESSED,  /**< Debounced press, including the press that woke the device. */
    BOARD_BUTTON_LONG,     /**< 1500 ms after the press, once per hold. */
    BOARD_BUTTON_SHORT,    /**< Release without LONG; reported before RELEASED. */
    BOARD_BUTTON_RELEASED, /**< Debounced release. */
} board_button_event_type_t;

typedef struct {
    board_button_t button;
    board_button_event_type_t type;
    int64_t timestamp_us; /**< esp_timer time when the debounced state was observed. */
} board_button_event_t;

/**
 * Receives events in the esp_timer task and must return without blocking.
 * The event is borrowed only for the callback; copy it before returning if needed.
 */
typedef void (*board_button_sink_fn)(void* ctx, const board_button_event_t* event);

/**
 * Initializes debounced buttons and light-sleep wake handling.
 *
 * Call once from startup task context after board_init_safe_gpio(). Debounce is
 * 50 ms; ESP_PM_NO_LIGHT_SLEEP is held while any button is pressed. A button held
 * at initialization yields no SHORT/LONG until released. On error, buttons
 * created before the failure remain active and may keep reporting to the sink.
 *
 * @param sink Non-NULL callback invoked from the esp_timer task.
 * @param ctx Borrowed callback context; must remain valid for the rest of the boot,
 * including after a partially failed initialization.
 * @return ESP_OK, ESP_ERR_INVALID_ARG for a NULL sink, ESP_ERR_INVALID_STATE
 * for repeated initialization after the sink was installed, or a setup error.
 */
esp_err_t board_buttons_init(board_button_sink_fn sink, void* ctx);
