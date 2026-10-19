idf_component_register(
    SRCS "board.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_adc esp_driver_gpio
    PRIV_REQUIRES esp_hw_support esp_timer esp_rom freertos log
)
