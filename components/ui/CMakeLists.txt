set(UI_SRCS
    "src/images.c"
    "src/fonts.c"
    "src/screens.c"
    "src/ui.c"
    "src/ui_navigation.c"
    "src/ui_special.c"
    "src/ui_values.c"
)

idf_component_register(
    SRCS ${UI_SRCS}
    INCLUDE_DIRS "include"
    REQUIRES lvgl
)
