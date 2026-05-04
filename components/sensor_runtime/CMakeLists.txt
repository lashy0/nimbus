idf_component_register(
    SRCS "src/sensor_runtime.c"
    INCLUDE_DIRS "include"
    REQUIRES bme680_sensor power_manager buttons app ui esp_lvgl_port esp_timer freertos
)
