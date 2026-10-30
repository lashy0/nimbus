idf_component_register(
    SRCS
        "ui.c"
        "view/ui_view.c"
        "view/ui_widgets.c"
        "view/font/ui_font_semibold_15.c"
        "view/font/ui_font_semibold_30.c"
        "view/font/ui_font_semibold_60.c"
        "view/page/ui_page_error.c"
        "view/page/ui_page_iaq.c"
        "view/page/ui_page_start.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "view" "view/font" "view/page"
    REQUIRES display
    PRIV_REQUIRES esp_lcd sensing
)
