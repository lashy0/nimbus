#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#include "display.h"

/**
 * @file
 * Screen content. LVGL runs in the esp_lvgl_port task; only that task
 * touches LVGL after start. Reads sensing and the buttons; sensing may start
 * before or after the UI, the buttons before it.
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
