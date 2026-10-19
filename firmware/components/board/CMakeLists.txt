idf_component_register(
    SRCS "board.c" "board_backlight.c" "board_buttons.c" "board_display.c" "board_i2c.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_i2c esp_lcd
    PRIV_REQUIRES esp_driver_gpio esp_driver_ledc esp_driver_spi esp_pm esp_timer log
)
