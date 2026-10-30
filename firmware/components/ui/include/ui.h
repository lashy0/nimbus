#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#include "display.h"

/**
 * @file
 * Screen content. LVGL runs in the esp_lvgl_port task; only that task
 * touches LVGL after start. Reads sensing, power and the buttons; sensing and
 * power may start before or after the UI, the buttons before it.
 *
 * Without presses the screen dims after 30 s and goes dark after 60 s; the
 * next press only brings it back.
 */

/**
 * Starts LVGL on the display. The LVGL task draws the first frame, then turns
 * output and the backlight on; a failure there is only logged.
 *
 * Call once, from a task. The display handles are owned by LVGL from now on.
 * Mount the assets partition at /assets first; without it the screens show
 * no images. Initialize NVS first; without it the brightness starts at its
 * default and is not kept.
 *
 * @param display Open display with nothing drawn on it.
 * @param core Core for the LVGL task, or tskNO_AFFINITY.
 * @retval ESP_OK LVGL runs; the first frame follows shortly.
 * @retval ESP_ERR_INVALID_ARG The chip has no such core.
 * @retval ESP_ERR_INVALID_STATE The UI has already started.
 * @retval ESP_ERR_NO_MEM LVGL buffers could not be allocated.
 * @retval ESP_FAIL The LVGL task could not be created.
 */
esp_err_t ui_start(const display_t* display, BaseType_t core);

/**
 * Whether the user has confirmed powering off on screen. Stays true; the UI
 * ignores the buttons from then on and waits for ui_stop().
 *
 * Call from any task.
 */
bool ui_power_off_requested(void);

/**
 * Stops drawing, saves a pending brightness change and turns the backlight
 * and the panel off. The UI cannot start again until a reset.
 *
 * Call from a task other than the LVGL one.
 *
 * @retval ESP_OK The screen is dark and the panel sleeps.
 * @retval ESP_ERR_INVALID_STATE The UI is not running.
 * @retval other The panel did not turn off; drawing stays stopped.
 */
esp_err_t ui_stop(void);
