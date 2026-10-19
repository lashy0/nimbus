idf_component_register(
    SRCS "ui.c" "ui_bringup.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_lcd
    PRIV_REQUIRES esp_pm esp_timer heap log
)
