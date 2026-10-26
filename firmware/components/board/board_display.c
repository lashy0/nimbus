#include "board_display.h"

#include "driver/spi_common.h"
#include "esp_check.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"

#include "board_pins.h"

#define LCD_HOST SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
// The 135x240 glass sits inside the 240x320 ST7789 memory. In the native portrait
// orientation (MADCTL 0, buttons below the glass) it starts at column 52, row 40.
#define LCD_X_GAP 52
#define LCD_Y_GAP 40

static const char* TAG = "board_display";

esp_err_t board_display_init(uint32_t max_transfer_bytes, board_display_t* out)
{
    ESP_RETURN_ON_FALSE(out != NULL && max_transfer_bytes > 0, ESP_ERR_INVALID_ARG, TAG, "arguments");

    const spi_bus_config_t bus_config = {
        .mosi_io_num = BOARD_PIN_LCD_MOSI,
        .miso_io_num = -1, // GPIO21 is I2C SDA.
        .sclk_io_num = BOARD_PIN_LCD_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = max_transfer_bytes,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(LCD_HOST, &bus_config, SPI_DMA_CH_AUTO), TAG, "SPI bus");

    const esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = BOARD_PIN_LCD_CS,
        .dc_gpio_num = BOARD_PIN_LCD_DC,
        .spi_mode = 0,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    esp_lcd_panel_io_handle_t panel_io = NULL;
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &panel_io), TAG, "panel IO");

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BOARD_PIN_PANEL_RESET,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    esp_lcd_panel_handle_t panel = NULL;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel), TAG, "ST7789");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel), TAG, "panel reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel), TAG, "panel init");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(panel, true), TAG, "invert");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_set_gap(panel, LCD_X_GAP, LCD_Y_GAP), TAG, "gap");

    *out = (board_display_t){
        .panel_io = panel_io,
        .panel = panel,
        .width = BOARD_DISPLAY_WIDTH,
        .height = BOARD_DISPLAY_HEIGHT,
        .swap_xy = false,
        .mirror_x = false,
        .mirror_y = false,
    };

    return ESP_OK;
}
