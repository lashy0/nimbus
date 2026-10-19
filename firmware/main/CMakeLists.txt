idf_component_register(
    SRCS "app_main.c" "composition.c"
    PRIV_REQUIRES app board esp_app_format esp_system espcoredump log sensing spiffs ui
)

# assets/ is generated from artwork/; flashed to the storage partition.
spiffs_create_partition_image(storage ${CMAKE_CURRENT_SOURCE_DIR}/../assets FLASH_IN_PROJECT)
