idf_component_register(
    SRCS "app.c" "app_state.c"
    INCLUDE_DIRS "include"
    REQUIRES sensing ui
    PRIV_REQUIRES esp_timer log storage
)
