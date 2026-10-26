#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_lcd_types.h"

#include "ui_model.h"

#define UI_DRAW_BUFFER_LINES 20 /**< LVGL buffer height in lines; the SPI bus must accept one buffer. */

/** Sets backlight to 0..100% synchronously from the calling task; ctx is borrowed. */
typedef esp_err_t (*ui_backlight_fn)(void* ctx, uint8_t percent);
/** Changes backlight pin ownership synchronously from the calling task; ctx is borrowed. */
typedef esp_err_t (*ui_backlight_pin_fn)(void* ctx);

/** Panel handles and callback context are borrowed and must remain valid for the rest of the boot. */
typedef struct {
    esp_lcd_panel_io_handle_t panel_io;
    esp_lcd_panel_handle_t panel;
    uint16_t width;
    uint16_t height;
    bool swap_xy;
    bool mirror_x;
    bool mirror_y;
    ui_backlight_fn set_backlight; /**< Nonzero brightness is applied only after the first frame reaches the panel. */
    /** OFF: the backlight is at 0; stop the PWM and keep its pin LOW through sleep. */
    ui_backlight_pin_fn release_backlight;
    /** Leaving OFF: the pin stays LOW and the PWM is ready at duty 0. */
    ui_backlight_pin_fn reclaim_backlight;
    void* backlight_ctx;        /**< Passed to all three backlight callbacks. */
    uint8_t brightness_percent; /**< Initial backlight level, 0..100%. */
} ui_config_t;

/**
 * Starts LVGL and shows the start screen before enabling the backlight.
 *
 * Call once per boot from a task on the core that initialized the panel's SPI bus;
 * the LVGL task is pinned to it. Waits for initialization and the first frame.
 * Configuration values are copied; panel handles and callback context stay borrowed.
 * Assets are read from S:/spiffs/. Missing fonts use the built-in font; a missing
 * animation timer uses a static image.
 *
 * On failure public UI calls stay unavailable and cleanup attempts to darken the
 * backlight. Resources are retained if a transfer cannot be stopped safely;
 * final shutdown must use the board sleep path. Initialization cannot be retried
 * after a valid configuration has begun startup in this boot.
 *
 * @param config Non-NULL configuration with panel handles, all backlight callbacks
 * and initial brightness in 0..100%.
 * @return ESP_OK, ESP_ERR_INVALID_ARG for invalid configuration,
 * ESP_ERR_INVALID_STATE for a repeated startup, or an initialization/transfer error.
 */
esp_err_t ui_init(const ui_config_t* config);

/**
 * Applies the model, redrawing only changed fields; the input is not retained.
 *
 * Call from task context after ui_init(). May wait for the LVGL lock. While the
 * display is OFF, only the task that turned it off can update the model; the
 * updated model is drawn on wake. Measurement values must already be rounded
 * and within the ranges described in ui_model.h.
 *
 * @param model Non-NULL model owned by the caller.
 * @retval ESP_OK The model was applied.
 * @retval ESP_ERR_INVALID_ARG The model is NULL.
 * @retval ESP_ERR_TIMEOUT LVGL is busy; the caller can retry later.
 * @retval ESP_ERR_INVALID_STATE UI is unavailable or rendering was stopped by another task.
 */
esp_err_t ui_apply_model(const ui_model_t* model);

/**
 * Sets display power state and the requested ON/DIM brightness.
 *
 * Call from one task after ui_init(); waits for rendering, transfers and panel
 * transitions. OFF stops LVGL, sleeps the panel, hands the backlight pin over
 * and releases the light-sleep lock. Leaving OFF draws the current model before
 * showing it and enabling the backlight. A repeated request for the reached
 * state and brightness requires no transition.
 *
 * An error means the request was not completed. A timeout while acquiring the
 * LVGL lock for OFF leaves the display unchanged. Later failures retain completed
 * steps, and the next call resumes the transition. The light-sleep lock is released
 * only after rendering stops and the panel and backlight reach OFF. OFF retains
 * the calling task's LVGL lock until wake, so another task cannot take over.
 *
 * @param state Requested display state.
 * @param brightness_percent Backlight level, 0..100%; validated but unused for OFF.
 * @return ESP_OK, ESP_ERR_INVALID_ARG for invalid arguments,
 * ESP_ERR_INVALID_STATE if UI is unavailable or another task owns stopped rendering,
 * ESP_ERR_TIMEOUT for a rendering/transfer timeout, or a panel/backlight/PM error.
 */
esp_err_t ui_set_display_state(ui_display_state_t state, uint8_t brightness_percent);
