#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_lcd_types.h"
#include "ui_model.h"

// LVGL draw buffer height in lines; the panel SPI bus must accept one such buffer.
#define UI_DRAW_BUFFER_LINES 20

typedef esp_err_t (*ui_backlight_fn)(void* ctx, uint8_t percent);

typedef struct {
    esp_lcd_panel_io_handle_t panel_io;
    esp_lcd_panel_handle_t panel;
    uint16_t width;
    uint16_t height;
    bool swap_xy;
    bool mirror_x;
    bool mirror_y;
    ui_backlight_fn set_backlight; // Called only after the first frame reached the panel.
    void* backlight_ctx;
    uint8_t brightness_percent;
} ui_config_t;

typedef enum {
    UI_DISPLAY_ON,
    UI_DISPLAY_DIM,
    UI_DISPLAY_OFF,
} ui_display_state_t;

// Starts LVGL, shows the start screen and turns the backlight on after its first
// flush. On error the backlight stays at 0. Assets are read from S:/spiffs/.
esp_err_t ui_init(const ui_config_t* config);

// Updates the screen to the model; only fields that differ are redrawn. Returns
// ESP_ERR_TIMEOUT when LVGL is busy (the caller retries later) and
// ESP_ERR_INVALID_STATE when the display was not initialized.
esp_err_t ui_apply_model(const ui_model_t* model);

// ON and DIM set the backlight to brightness_percent, which the caller computes for
// the state. OFF returns ESP_ERR_NOT_SUPPORTED: the LVGL port cannot quiesce yet.
esp_err_t ui_set_display_state(ui_display_state_t state, uint8_t brightness_percent);
