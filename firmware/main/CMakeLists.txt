set(srcs "app_main.c" "composition.c")
if(CONFIG_NIMBUS_PREVIEW_READINGS)
    list(APPEND srcs "preview_readings.c")
endif()

idf_component_register(
    SRCS ${srcs}
    PRIV_REQUIRES app board esp_app_format esp_system esp_timer espcoredump log spiffs ui
)

# assets/ is generated from artwork/; flashed to the storage partition.
spiffs_create_partition_image(storage ${CMAKE_CURRENT_SOURCE_DIR}/../assets FLASH_IN_PROJECT)
