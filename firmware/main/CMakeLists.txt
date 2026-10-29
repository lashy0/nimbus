idf_component_register(
    SRCS "nimbus.c"
    INCLUDE_DIRS "."
    # espcoredump: the minimal build drops it unless a component requires it.
    PRIV_REQUIRES esp_app_format esp_driver_i2c espcoredump log nvs_flash sensing
)
