#include "display.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

#define PIN_NUM_MISO 21
#define PIN_NUM_MOSI 19
#define PIN_NUM_SCLK 18
#define PIN_NUM_CS 5
#define PIN_NUM_DC 16
#define PIN_NUM_RST 23

#define LCD_HOST SPI2_HOST
#define SPI_CLOCK_HZ (27 * 1000 * 1000)

display_handles_t display_init(void)
{
    display_handles_t handles = {0};

    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 135 * 240 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = SPI_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &handles.io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(handles.io_handle, &panel_config, &handles.panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(handles.panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(handles.panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(handles.panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(handles.panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(handles.panel_handle, false, true));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(handles.panel_handle, 53, 40));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(handles.panel_handle, true));

    return handles;
}