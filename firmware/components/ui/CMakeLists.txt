idf_component_register(
    SRCS
        "ui.c"
        "view/ui_view.c"
        "view/font/ui_font_semibold_15.c"
        "view/font/ui_font_semibold_30.c"
        "view/font/ui_font_semibold_60.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "view" "view/font"
    REQUIRES display
    PRIV_REQUIRES esp_lcd
)
