set(srcs "src/power_manager.c")
set(includes "include")

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${includes}
    REQUIRES driver esp_lvgl_port ui backlight bme680_sensor
)
