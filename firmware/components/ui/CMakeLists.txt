idf_component_register(
    SRCS
        "ui.c"
        "display/ui_display.c"
        "display/ui_render.c"
        "view/ui_assets.c"
        "view/ui_blink.c"
        "view/ui_charge.c"
        "view/ui_faces.c"
        "view/ui_fonts.c"
        "view/ui_view.c"
        "view/ui_widgets.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "display" "view"
    REQUIRES esp_lcd
    PRIV_REQUIRES esp_pm esp_timer heap log
)
