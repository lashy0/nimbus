#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_lcd_types.h"

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

// Starts LVGL, shows the bring-up screen and turns the backlight on after its first
// flush. On error the backlight stays at 0. Assets are read from S:/spiffs/.
esp_err_t ui_init(const ui_config_t* config);
