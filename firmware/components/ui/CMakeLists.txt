idf_component_register(
    SRCS "ui.c"
    INCLUDE_DIRS "include"
    REQUIRES display
    PRIV_REQUIRES esp_lcd
)
