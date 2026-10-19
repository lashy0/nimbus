#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_lcd_types.h"

#define BOARD_DISPLAY_WIDTH 135 // Portrait, buttons below the glass.
#define BOARD_DISPLAY_HEIGHT 240

typedef struct {
    esp_lcd_panel_io_handle_t panel_io;
    esp_lcd_panel_handle_t panel;
    uint16_t width;  // BOARD_DISPLAY_WIDTH
    uint16_t height; // BOARD_DISPLAY_HEIGHT
    bool swap_xy;    // Rotation to apply through esp_lcd; offsets already match it.
    bool mirror_x;
    bool mirror_y;
} board_display_t;

// SPI bus, panel IO and ST7789 reset/init. The panel is left with display off;
// the backlight is not touched. The handles live for the whole boot.
esp_err_t board_display_init(uint32_t max_transfer_bytes, board_display_t* out);
