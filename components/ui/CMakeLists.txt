file(GLOB UI_SRCS "src/*.c")

idf_component_register(
    SRCS ${UI_SRCS}
    INCLUDE_DIRS "include"
    REQUIRES lvgl
)
