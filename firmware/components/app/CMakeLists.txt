idf_component_register(
    SRCS "app.c" "app_display.c" "app_model.c" "app_state.c"
    INCLUDE_DIRS "include"
    REQUIRES power sensing ui
    PRIV_REQUIRES esp_timer log storage
)
