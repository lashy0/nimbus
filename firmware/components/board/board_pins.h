#pragma once

#include <stdbool.h>

#include "board_buttons.h"
#include "driver/gpio.h"

#define BOARD_PIN_BACKLIGHT GPIO_NUM_4
#define BOARD_PIN_ADC_ENABLE GPIO_NUM_14
#define BOARD_PIN_BUTTON_PREV GPIO_NUM_0
#define BOARD_PIN_BUTTON_NEXT GPIO_NUM_35

#define BOARD_PIN_LCD_MOSI GPIO_NUM_19
#define BOARD_PIN_LCD_SCLK GPIO_NUM_18
#define BOARD_PIN_LCD_CS GPIO_NUM_5
#define BOARD_PIN_LCD_DC GPIO_NUM_16
#define BOARD_PIN_PANEL_RESET GPIO_NUM_23

#define BOARD_PIN_I2C_SDA GPIO_NUM_21
#define BOARD_PIN_I2C_SCL GPIO_NUM_22

// Both buttons are active LOW.
static inline gpio_num_t board_button_pin(board_button_t button)
{
    return button == BOARD_BUTTON_PREV ? BOARD_PIN_BUTTON_PREV : BOARD_PIN_BUTTON_NEXT;
}

// GPIO35 is input-only without internal pulls; the board pulls it up externally.
static inline bool board_button_internal_pull_up(board_button_t button)
{
    return button == BOARD_BUTTON_PREV;
}
