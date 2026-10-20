#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_lcd_types.h"
#include "ui_model.h"

// LVGL draw buffer height in lines; the panel SPI bus must accept one such buffer.
#define UI_DRAW_BUFFER_LINES 20

typedef esp_err_t (*ui_backlight_fn)(void* ctx, uint8_t percent);
typedef esp_err_t (*ui_backlight_pin_fn)(void* ctx);

typedef struct {
    esp_lcd_panel_io_handle_t panel_io;
    esp_lcd_panel_handle_t panel;
    uint16_t width;
    uint16_t height;
    bool swap_xy;
    bool mirror_x;
    bool mirror_y;
    ui_backlight_fn set_backlight; // Called only after the first frame reached the panel.
    // OFF: the backlight is at 0; stop the PWM and keep its pin LOW through sleep.
    ui_backlight_pin_fn release_backlight;
    // Leaving OFF: the pin stays LOW and the PWM is ready at duty 0.
    ui_backlight_pin_fn reclaim_backlight;
    void* backlight_ctx; // Passed to all three backlight callbacks.
    uint8_t brightness_percent;
} ui_config_t;

// Starts LVGL, shows the start screen and turns the backlight on after its first
// flush. On error the backlight stays at 0. Assets are read from S:/spiffs/.
// Call on the core that initialized the panel's SPI bus: the LVGL task is pinned to it.
esp_err_t ui_init(const ui_config_t* config);

// Updates the screen to the model; only fields that differ are redrawn. Returns
// ESP_ERR_TIMEOUT when LVGL is busy (the caller retries later) and
// ESP_ERR_INVALID_STATE when the display was not initialized. While the display is
// OFF only the task that turned it off can update the model; it is drawn on wake.
esp_err_t ui_apply_model(const ui_model_t* model);

// Moves the display to state; brightness_percent is the backlight for ON and DIM,
// which the caller computes, and is ignored for OFF. Repeating the reached state
// does nothing. OFF stops LVGL, sleeps the panel, hands the backlight pin over and
// releases the light-sleep lock; leaving OFF draws the current model before the
// panel shows it and lights the backlight only after that.
//
// An error means the state was not reached: ESP_ERR_TIMEOUT from OFF while LVGL
// was busy leaves the display as it was; later failures leave it dark and keep
// the light-sleep lock, and the next call continues from the steps already done.
// OFF keeps the LVGL lock of the calling task until the display wakes, so call
// from one task only; another task gets ESP_ERR_INVALID_STATE.
esp_err_t ui_set_display_state(ui_display_state_t state, uint8_t brightness_percent);
