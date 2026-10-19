idf_component_register(
    SRCS "app_main.c" "composition.c"
    PRIV_REQUIRES board esp_app_format esp_system espcoredump log
)
