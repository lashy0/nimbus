#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

/**
 * @file
 * Pauses LVGL while the screen is dark, so the chip can light-sleep.
 */

/** Starts the task that holds LVGL during a pause. */
esp_err_t ui_pause_start(BaseType_t core);

/**
 * Pauses LVGL once the LVGL task finishes its current pass. The pause ends when
 * a press is queued or the power source changes.
 *
 * Call from the LVGL task.
 */
void ui_pause_request(void);

/** Ends a pause within 0.5 s and refuses later requests. */
void ui_pause_disable(void);
