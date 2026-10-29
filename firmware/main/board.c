#include "board.h"

#include <stdint.h>

#include "driver/i2c_master.h"
#include "driver/spi_common.h"

#include "display.h"

esp_err_t board_create_i2c_bus(i2c_master_bus_handle_t* out)
{
    const i2c_master_bus_config_t config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = BOARD_PIN_I2C_SDA,
        .scl_io_num = BOARD_PIN_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        // The breakout has its own pull-ups; the internal ones only add margin.
        .flags.enable_internal_pullup = true,
    };

    return i2c_new_master_bus(&config, out);
}

esp_err_t board_create_spi_bus(void)
{
    const spi_bus_config_t config = {
        .mosi_io_num = BOARD_PIN_LCD_MOSI,
        .miso_io_num = -1, // The panel is write-only here.
        .sclk_io_num = BOARD_PIN_LCD_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t),
    };

    return spi_bus_initialize(BOARD_LCD_SPI_HOST, &config, SPI_DMA_CH_AUTO);
}
