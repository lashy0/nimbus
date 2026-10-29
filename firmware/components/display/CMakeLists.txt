idf_component_register(
    SRCS "display.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_lcd
    PRIV_REQUIRES esp_driver_ledc log
)
