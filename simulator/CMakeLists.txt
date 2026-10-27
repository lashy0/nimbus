cmake_minimum_required(VERSION 3.24)
project(lvgl C CXX)

# Let clangd use the same includes and defines as the desktop build.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Limit parallel compilation and linking with Ninja.
set(NIMBUS_BUILD_JOBS 2 CACHE STRING "Maximum parallel compilation jobs")
set_property(GLOBAL PROPERTY JOB_POOLS compile_pool=${NIMBUS_BUILD_JOBS} link_pool=1)
set(CMAKE_JOB_POOL_COMPILE compile_pool)
set(CMAKE_JOB_POOL_LINK link_pool)

# Set the correct FreeRTOS port for your system (e.g., Posix for WSL)
if(WIN32)
    set(FREERTOS_PORT MSVC_MINGW CACHE STRING "Port for FreeRTOS on Windows")
else()
    set(FREERTOS_PORT GCC_POSIX CACHE STRING "Port for FreeRTOS on Posix environment")
endif()

option(USE_FREERTOS "Enable FreeRTOS" OFF) # Turn this on to enable FreeRTOS

if(USE_FREERTOS)
    message(STATUS "FreeRTOS is enabled")

    # FreeRTOS integration when USE_FREERTOS is enabled
    add_library(freertos_config INTERFACE)
    target_include_directories(freertos_config SYSTEM INTERFACE ${PROJECT_SOURCE_DIR}/config)
    target_compile_definitions(freertos_config INTERFACE projCOVERAGE_TEST=0)

    # Add FreeRTOS as a subdirectory
    set(FREERTOS_HEAP 4 CACHE STRING "FreeRTOS heap implementation")
    add_subdirectory(FreeRTOS)
    add_compile_definitions(NIMBUS_USE_FREERTOS)

    # FreeRTOS-specific include directories
    include_directories(${PROJECT_SOURCE_DIR}/FreeRTOS/include)
    include_directories(${PROJECT_SOURCE_DIR}/config)

else()
    message(STATUS "FreeRTOS is disabled")
endif()

# Main include files of the project
include_directories(${PROJECT_SOURCE_DIR}/main/inc)

# Define options for LVGL with default values (OFF)
option(LV_USE_DRAW_SDL "Use SDL draw unit" OFF)
option(LV_USE_LIBPNG "Use libpng to decode PNG" OFF)
option(LV_USE_LIBJPEG_TURBO "Use libjpeg turbo to decode JPEG" OFF)
option(LV_USE_FFMPEG "Use libffmpeg to display video using lv_ffmpeg" OFF)
option(LV_USE_FREETYPE "Use freetype library" OFF)

# Set C and C++ standards
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the output path for the executable
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

# Find and include SDL2 library
find_package(SDL2 2.32.10 EXACT QUIET CONFIG)
if(NOT SDL2_FOUND)
    include(FetchContent)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL2
        URL https://codeload.github.com/libsdl-org/SDL/tar.gz/refs/tags/release-2.32.10
        URL_HASH SHA256=03f9d7c191a837525c9cda6406af2f2e48be02b5e7eb03d949cc9f1e9ca41c8b
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(SDL2)
    set(SDL2_INCLUDE_DIRS "${sdl2_SOURCE_DIR}/include")
endif()
set(SDL2_LIBRARIES SDL2::SDL2-static)

# Add compile definitions based on the selected options
add_compile_definitions($<$<BOOL:${LV_USE_DRAW_SDL}>:LV_USE_DRAW_SDL=1>)
add_compile_definitions($<$<BOOL:${LV_USE_LIBPNG}>:LV_USE_LIBPNG=1>)
add_compile_definitions($<$<BOOL:${LV_USE_LIBJPEG_TURBO}>:LV_USE_LIBJPEG_TURBO=1>)
add_compile_definitions($<$<BOOL:${LV_USE_FFMPEG}>:LV_USE_FFMPEG=1>)

# Add LVGL subdirectory
set(LV_BUILD_CONF_PATH "${PROJECT_SOURCE_DIR}/lv_conf.h" CACHE PATH "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
add_subdirectory(lvgl)
target_include_directories(lvgl PUBLIC ${PROJECT_SOURCE_DIR} ${SDL2_INCLUDE_DIRS})
target_link_libraries(lvgl PUBLIC ${SDL2_LIBRARIES})

# Create the main executable, depending on the FreeRTOS option
if(USE_FREERTOS)
    add_executable(main
        ${PROJECT_SOURCE_DIR}/main/src/main.c
        ${PROJECT_SOURCE_DIR}/main/src/freertos_main.cpp
        ${PROJECT_SOURCE_DIR}/main/src/mouse_cursor_icon.c
    )
    # Link FreeRTOS libraries
    target_link_libraries(main freertos_config freertos_kernel)
else()
    add_executable(main
        ${PROJECT_SOURCE_DIR}/main/src/main.c
        ${PROJECT_SOURCE_DIR}/main/src/mouse_cursor_icon.c
    )
endif()

set_target_properties(main PROPERTIES OUTPUT_NAME "nimbus-sim")

# SDL uses the first embedded Windows icon for the window as well as Explorer.
if(WIN32)
    enable_language(RC)
    target_sources(main PRIVATE assets/nimbus.rc)
    target_include_directories(main PRIVATE "${PROJECT_SOURCE_DIR}/assets")
    set_source_files_properties(assets/nimbus.rc PROPERTIES
        OBJECT_DEPENDS "${PROJECT_SOURCE_DIR}/assets/nimbus.ico"
    )
endif()

# Use the same UI sources and assets as the firmware.
set(UI_DIR "${PROJECT_SOURCE_DIR}/../firmware/components/ui")
set(APP_DIR "${PROJECT_SOURCE_DIR}/../firmware/components/app")

# Compile the hardware-independent policy directly from the firmware.
add_library(preview_app STATIC
    main/src/preview.c
    "${APP_DIR}/app_state.c"
    "${APP_DIR}/app_display.c"
    "${APP_DIR}/app_model.c"
)
target_include_directories(preview_app PUBLIC
    main/src
    "${APP_DIR}/include"
    "${UI_DIR}/include"
    "${PROJECT_SOURCE_DIR}/../firmware/components/power/include"
    "${PROJECT_SOURCE_DIR}/../firmware/components/sensing/include"
)
if(NOT MSVC)
    target_link_libraries(preview_app PUBLIC m)
endif()
target_link_libraries(main preview_app)

target_sources(main PRIVATE
    "${UI_DIR}/view/ui_assets.c"
    "${UI_DIR}/view/ui_blink.c"
    "${UI_DIR}/view/ui_charge.c"
    "${UI_DIR}/view/ui_faces.c"
    "${UI_DIR}/view/ui_fonts.c"
    "${UI_DIR}/view/ui_pulse.c"
    "${UI_DIR}/view/ui_view.c"
    "${UI_DIR}/view/ui_widgets.c"
)
target_include_directories(main PRIVATE "${UI_DIR}/include" "${UI_DIR}/view")
target_compile_definitions(main PRIVATE SDL_MAIN_HANDLED UI_ASSET_ROOT="S:")
target_compile_definitions(lvgl PUBLIC NIMBUS_ASSETS_DIR="${PROJECT_SOURCE_DIR}/../firmware/assets")

# Define LVGL configuration as a simple include
target_compile_definitions(main PRIVATE LV_CONF_INCLUDE_SIMPLE)
target_link_libraries(main lvgl ${SDL2_LIBRARIES})
if(NOT WIN32)
    target_link_libraries(main m pthread)
endif()
if(MSVC)
    target_compile_options(main PRIVATE /utf-8)
    target_compile_options(lvgl PRIVATE /utf-8)
endif()

# Only link freertos_config if the FreeRTOS directory exists
if(USE_FREERTOS)
    target_link_libraries(main freertos_config)
    target_link_libraries(lvgl PUBLIC freertos_kernel)
    if(NOT WIN32)
        target_sources(main PRIVATE ${PROJECT_SOURCE_DIR}/main/src/FreeRTOS_Posix_Port.c)
    endif()
endif()

# Custom target to run the executable
add_custom_target(run COMMAND $<TARGET_FILE:main> DEPENDS main)

# Conditionally include and link SDL2_image if LV_USE_DRAW_SDL is enabled
if(LV_USE_DRAW_SDL)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
    find_package(SDL2_image REQUIRED)
    target_include_directories(lvgl PUBLIC ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_libraries(main ${SDL2_IMAGE_LIBRARIES})
endif()

# Conditionally include and link libpng if LV_USE_LIBPNG is enabled
if(LV_USE_LIBPNG)
    find_package(PNG REQUIRED)
    target_include_directories(lvgl PUBLIC ${PNG_INCLUDE_DIRS})
    target_link_libraries(main ${PNG_LIBRARIES})
endif()

# Conditionally include and link libjpeg-turbo if LV_USE_LIBJPEG_TURBO is enabled
if(LV_USE_LIBJPEG_TURBO)
    find_package(JPEG REQUIRED)
    target_include_directories(lvgl PUBLIC ${JPEG_INCLUDE_DIRS})
    target_link_libraries(main ${JPEG_LIBRARIES})
endif()

# Conditionally include and link FFmpeg libraries if LV_USE_FFMPEG is enabled
if(LV_USE_FFMPEG)
    target_link_libraries(main avformat avcodec avutil swscale z)
endif()

# Conditionally include and link FreeType if LV_USE_FREETYPE is enabled
if(LV_USE_FREETYPE)
    find_package(Freetype REQUIRED)
    target_include_directories(lvgl PUBLIC ${FREETYPE_INCLUDE_DIRS})
    target_link_libraries(main ${FREETYPE_LIBRARIES})
endif()

# Apply additional compile options if the build type is Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug mode enabled")

    target_compile_options(lvgl PRIVATE
        -pedantic-errors
        -Wall
        -Wclobbered
        -Wdeprecated
        -Wdouble-promotion
        -Wempty-body
        -Wextra
        -Wformat-security
        -Wmaybe-uninitialized
        # -Wmissing-prototypes
        -Wpointer-arith
        -Wmultichar
        -Wno-pedantic # ignored for now, we convert functions to pointers for properties table.
        -Wreturn-type
        -Wshadow
        -Wshift-negative-value
        -Wsizeof-pointer-memaccess
        -Wtype-limits
        -Wundef
        -Wuninitialized
        -Wunreachable-code
        -Wfloat-conversion
        -Wstrict-aliasing
    )

if (ASAN)
    message(STATUS "AddressSanitizer enabled")

    # Add AddressSanitizer flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
else()
    message(STATUS "AddressSanitizer disabled")
endif()
endif()
