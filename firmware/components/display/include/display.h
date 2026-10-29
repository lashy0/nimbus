#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "esp_lcd_types.h"
#include "hal/spi_types.h"
#include "soc/gpio_num.h"

/**
 * @file
 * ST7789 panel and its backlight. Brings the hardware up and hands out the
 * esp_lcd handles; drawing is up to the caller. One display per firmware.
 */

#define DISPLAY_WIDTH 135
#define DISPLAY_HEIGHT 240

typedef struct {
    spi_host_device_t spi_host; /**< Initialized bus; max_transfer_sz must fit the largest draw. */
    gpio_num_t cs;
    gpio_num_t dc;
    gpio_num_t reset;
    gpio_num_t backlight;
} display_config_t;

typedef struct {
    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_handle_t panel; /**< Origin at the top-left corner, buttons below the glass. */
} display_t;

/**
 * Initializes the panel and the backlight.
 *
 * The panel keeps its RAM through a reset, so it may hold an old frame.
 * Output stays off and the backlight at 0 %; draw a frame, then call
 * esp_lcd_panel_disp_on_off() and display_set_brightness().
 *
 * @param config Non-NULL pins and bus.
 * @param out Non-NULL destination; set only on ESP_OK.
 * @retval ESP_OK The panel is initialized.
 * @retval ESP_ERR_INVALID_STATE The display is already open.
 * @retval ESP_ERR_NO_MEM The panel IO or the panel could not be allocated.
 * @retval ESP_FAIL A panel command or the backlight setup failed.
 */
esp_err_t display_open(const display_config_t* config, display_t* out);

/**
 * Sets the backlight brightness.
 *
 * @param percent 0..100; 0 turns the backlight off.
 * @retval ESP_OK The new duty is applied.
 * @retval ESP_ERR_INVALID_ARG percent is above 100.
 * @retval ESP_ERR_INVALID_STATE The display is not open.
 */
esp_err_t display_set_brightness(uint8_t percent);
