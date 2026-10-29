idf_component_register(
    SRCS "ui.c" "view/ui_view.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "view"
    REQUIRES display
    PRIV_REQUIRES esp_lcd
)
