idf_component_register(
    SRCS
        "ui.c"
        "ui_settings.c"
        "view/ui_view.c"
        "view/ui_widgets.c"
        "view/font/ui_font_semibold_15.c"
        "view/font/ui_font_semibold_30.c"
        "view/font/ui_font_semibold_50.c"
        "view/font/ui_font_semibold_60.c"
        "view/page/ui_page_brightness.c"
        "view/page/ui_page_error.c"
        "view/page/ui_page_hum.c"
        "view/page/ui_page_iaq.c"
        "view/page/ui_page_start.c"
        "view/page/ui_page_temp.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "view" "view/font" "view/page"
    REQUIRES display
    PRIV_REQUIRES buttons esp_lcd nvs_flash sensing
)
