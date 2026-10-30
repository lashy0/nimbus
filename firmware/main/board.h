#pragma once

#include "driver/i2c_types.h"
#include "esp_err.h"
#include "hal/spi_types.h" // IWYU pragma: export
#include "soc/gpio_num.h"  // IWYU pragma: export

/**
 * @file
 * LILYGO T-Display (ESP32) with a BME680 breakout: pins and buses.
 */

#define BOARD_PIN_I2C_SDA GPIO_NUM_21
#define BOARD_PIN_I2C_SCL GPIO_NUM_22

#define BOARD_LCD_SPI_HOST SPI2_HOST
#define BOARD_PIN_LCD_MOSI GPIO_NUM_19
#define BOARD_PIN_LCD_SCLK GPIO_NUM_18
#define BOARD_PIN_LCD_CS GPIO_NUM_5
#define BOARD_PIN_LCD_DC GPIO_NUM_16
#define BOARD_PIN_LCD_RESET GPIO_NUM_23
#define BOARD_PIN_LCD_BACKLIGHT GPIO_NUM_4

// Active low. GPIO35 is input-only and pulled up on the board.
#define BOARD_PIN_BUTTON_PREV GPIO_NUM_0
#define BOARD_PIN_BUTTON_NEXT GPIO_NUM_35

/** Creates the I2C bus of the BME680. */
esp_err_t board_create_i2c_bus(i2c_master_bus_handle_t* out);

/** Initializes the display SPI bus on BOARD_LCD_SPI_HOST, sized for a full frame. */
esp_err_t board_create_spi_bus(void);
