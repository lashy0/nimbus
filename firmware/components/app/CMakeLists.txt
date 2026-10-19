idf_component_register(
    SRCS "app.c" "app_state.c"
    INCLUDE_DIRS "include"
    REQUIRES ui
    PRIV_REQUIRES esp_timer log
)
