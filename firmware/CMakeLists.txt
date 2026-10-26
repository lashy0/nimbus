cmake_minimum_required(VERSION 3.22)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)

set(PROJECT_VER "2.0.0-dev")
project(nimbus)

idf_component_get_property(nimbus_lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_definitions(${nimbus_lvgl_lib} PUBLIC
    LV_CONF_PATH="${CMAKE_CURRENT_LIST_DIR}/components/ui/lv_conf.h")
