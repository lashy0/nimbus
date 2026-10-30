idf_component_register(
    SRCS "board.c" "nimbus.c"
    INCLUDE_DIRS "."
    # espcoredump: the minimal build drops it unless a component requires it.
    PRIV_REQUIRES buttons display esp_app_format esp_driver_i2c esp_driver_spi espcoredump log nvs_flash power sensing spiffs ui
)
