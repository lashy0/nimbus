idf_component_register(
    SRCS "storage.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_rom log nvs_flash
)
