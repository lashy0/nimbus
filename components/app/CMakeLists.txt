set(srcs "src/app.c")
set(includes "include")

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${includes}
    REQUIRES esp_lvgl_port ui buttons power_manager display backlight
)
