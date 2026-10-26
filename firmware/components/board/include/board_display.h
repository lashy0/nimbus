#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_lcd_types.h"

#define BOARD_DISPLAY_WIDTH 135 /**< Portrait, buttons below the glass. */
#define BOARD_DISPLAY_HEIGHT 240

typedef struct {
    esp_lcd_panel_io_handle_t panel_io;
    esp_lcd_panel_handle_t panel;
    uint16_t width;  /**< BOARD_DISPLAY_WIDTH */
    uint16_t height; /**< BOARD_DISPLAY_HEIGHT */
    bool swap_xy;    /**< Rotation to apply through esp_lcd; offsets already match it. */
    bool mirror_x;
    bool mirror_y;
} board_display_t;

/**
 * Initializes the SPI bus, panel IO and ST7789, leaving the panel display off.
 *
 * Call once from startup task context after board_init_safe_gpio(); blocks during
 * panel reset and initialization. Backlight is not touched. Returned handles are
 * board-owned for the rest of the boot and may be lent to UI. On failure partial
 * hardware resources remain allocated; use the final board sleep path.
 *
 * @param max_transfer_bytes Nonzero maximum SPI transfer size in bytes; must fit a UI draw buffer.
 * @param out Non-NULL destination; unchanged on error.
 * @return ESP_OK, ESP_ERR_INVALID_ARG for invalid arguments, or a SPI/panel setup error.
 */
esp_err_t board_display_init(uint32_t max_transfer_bytes, board_display_t* out);
