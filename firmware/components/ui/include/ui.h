#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#include "display.h"

/**
 * @file
 * Screen content. LVGL runs in the esp_lvgl_port task; only that task
 * touches LVGL after start.
 */

/**
 * Starts LVGL on the display, draws the first frame, then turns output and
 * the backlight on.
 *
 * Call once, from a task. The display handles are owned by LVGL from now on.
 *
 * @param display Open display with nothing drawn on it.
 * @param core Core for the LVGL task, or tskNO_AFFINITY.
 * @retval ESP_OK The first frame is on the screen.
 * @retval ESP_ERR_INVALID_ARG The chip has no such core.
 * @retval ESP_ERR_INVALID_STATE The UI has already started.
 * @retval ESP_ERR_NO_MEM LVGL buffers or the task could not be allocated.
 * @retval ESP_FAIL The LVGL task or a panel command failed.
 */
esp_err_t ui_start(const display_t* display, BaseType_t core);
