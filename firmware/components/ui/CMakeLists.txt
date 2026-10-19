idf_component_register(
    SRCS "ui.c" "ui_blink.c" "ui_faces.c" "ui_fonts.c" "ui_view.c" "ui_widgets.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_lcd
    PRIV_REQUIRES esp_pm esp_timer heap log
)
