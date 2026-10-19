idf_component_register(
    SRCS "board.c" "board_buttons.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_driver_gpio esp_pm esp_timer log
)
