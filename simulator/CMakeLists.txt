cmake_minimum_required(VERSION 3.24)
project(nimbus_sim C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

get_filename_component(FIRMWARE_DIR ${CMAKE_CURRENT_LIST_DIR}/../firmware ABSOLUTE)
set(COMPONENTS_DIR ${FIRMWARE_DIR}/components)
set(UI_DIR ${COMPONENTS_DIR}/ui)

include(FetchContent)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL2
    URL https://codeload.github.com/libsdl-org/SDL/tar.gz/refs/tags/release-2.32.10
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# The same LVGL version as the firmware.
set(LV_BUILD_CONF_PATH ${CMAKE_CURRENT_LIST_DIR}/lv_conf.h CACHE PATH "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(lvgl
    URL https://github.com/lvgl/lvgl/archive/refs/tags/v9.5.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(SDL2 lvgl)
target_link_libraries(lvgl PUBLIC SDL2::SDL2-static)
target_compile_definitions(lvgl PUBLIC NIMBUS_ASSETS_DIR="${FIRMWARE_DIR}/assets/")
if(MSVC)
    target_compile_options(lvgl PRIVATE /wd5287) # Mixed enum operands in LVGL.
endif()

# Every view source, so new pages build here without changes.
file(GLOB VIEW_SOURCES CONFIGURE_DEPENDS
    ${UI_DIR}/view/*.c
    ${UI_DIR}/view/anim/*.c
    ${UI_DIR}/view/page/*.c
)

add_executable(nimbus-sim
    src/main.c
    src/sim_data.c
    src/sim_input.c
    src/sim_log.c
    src/sim_port.c
    ${VIEW_SOURCES}
)
target_include_directories(nimbus-sim PRIVATE
    src
    ${UI_DIR}/view
    ${UI_DIR}/view/anim
    ${UI_DIR}/view/page
    ${COMPONENTS_DIR}/buttons/include
    ${COMPONENTS_DIR}/power/include
    ${COMPONENTS_DIR}/sensing/include
)
target_link_libraries(nimbus-sim PRIVATE lvgl)

if(MSVC)
    target_compile_options(nimbus-sim PRIVATE /utf-8)
endif()

# SDL takes the window icon from the first icon of the executable.
if(WIN32)
    target_sources(nimbus-sim PRIVATE assets/nimbus.rc)
    set_source_files_properties(assets/nimbus.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/assets/nimbus.ico)
endif()
