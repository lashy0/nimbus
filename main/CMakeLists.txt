idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES lvgl esp_timer spiffs display backlight ui)

spiffs_create_partition_image(storage ../assets FLASH_IN_PROJECT)