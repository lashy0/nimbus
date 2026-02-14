idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES lvgl spiffs app buttons display backlight ui bme680_sensor)

spiffs_create_partition_image(storage ../assets FLASH_IN_PROJECT)
