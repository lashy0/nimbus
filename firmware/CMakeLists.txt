cmake_minimum_required(VERSION 3.22)

set(PROJECT_VER "v1.0.0")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
project(nimbus)

# LVGL settings Kconfig cannot set; lv_conf.h wins over Kconfig.
idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_definitions(${lvgl_lib} PUBLIC "LV_CONF_PATH=\"${CMAKE_CURRENT_LIST_DIR}/components/ui/lv_conf.h\"")

# Flashed together with the app, so the assets always match the code.
spiffs_create_partition_image(assets ${CMAKE_CURRENT_LIST_DIR}/assets FLASH_IN_PROJECT)
