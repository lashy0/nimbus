idf_component_register(
    SRCS "app_main.c" "composition.c" "restart_guard.c"
    PRIV_REQUIRES app board esp_app_format esp_hw_support esp_pm esp_system esp_timer espcoredump log power sensing spiffs storage ui
)

# assets/; flashed to the storage partition.
spiffs_create_partition_image(storage ${CMAKE_CURRENT_SOURCE_DIR}/../assets FLASH_IN_PROJECT)
