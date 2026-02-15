set(UI_SRCS
    "src/images.c"
    "src/screens.c"
    "src/ui.c"
    "src/ui_navigation.c"
    "src/ui_special.c"
    "src/ui_values.c"
    "src/ui_font_sf_b_10_digits.c"
    "src/ui_font_sf_sb_30_digits.c"
    "src/ui_font_sf_sb_50_digits.c"
    "src/ui_font_sf_sb_60_digits.c"
)

idf_component_register(
    SRCS ${UI_SRCS}
    INCLUDE_DIRS "include"
    REQUIRES lvgl
)
