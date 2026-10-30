#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "soc/gpio_num.h"

#include "buttons_types.h"

/**
 * @file
 * The two buttons below the display.
 */

/** Pins are active low. A pin without an internal pull-up needs one on the board. */
typedef struct {
    gpio_num_t prev;
    gpio_num_t next;
} buttons_config_t;

/**
 * Configures the pins and starts watching them.
 *
 * Call once, from a task, before the reader starts. On failure a button set
 * up before the error keeps reporting.
 *
 * @param config Non-NULL pins.
 * @retval ESP_OK Presses are being queued.
 * @retval ESP_ERR_INVALID_STATE The buttons have already started.
 * @retval ESP_ERR_NO_MEM The queue or a button could not be allocated.
 * @retval ESP_ERR_INVALID_ARG A pin cannot be used as an input.
 */
esp_err_t buttons_start(const buttons_config_t* config);

/**
 * Takes the oldest press without waiting.
 *
 * Call from one task only. Presses that do not fit the queue are dropped.
 *
 * @param out Non-NULL destination; set only when true is returned.
 * @return false if nothing is queued or the buttons have not started.
 */
bool buttons_receive(buttons_event_t* out);
