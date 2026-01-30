#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

#define WIDTH 135
#define HEIGHT 240

#define PIN_NUM_BL 4

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Structure containing the hardware handles for the display
 */
typedef struct {
    esp_lcd_panel_io_handle_t io_handle;    /*!< Handle for the LCD IO interface (SPI) */
    esp_lcd_panel_handle_t    panel_handle; /*!< Handle for the LCD panel controller */
} display_handles_t;

/**
 * @brief Initialize the display hardware
 *
 * @return display_handles_t Structure containing initialized IO and Panel handles.
 */
display_handles_t display_init(void);

#ifdef __cplusplus
}
#endif