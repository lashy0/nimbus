set(srcs "src/buttons.c")
set(includes "include")

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${includes}
    REQUIRES driver button esp_timer
)
