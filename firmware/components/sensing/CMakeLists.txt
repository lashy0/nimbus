set(bsec_dir ${CMAKE_CURRENT_LIST_DIR}/../../3rdparty/bsec2/src)
set(bme68x_dir ${CMAKE_CURRENT_LIST_DIR}/../../3rdparty/bme68x)
set(bsec_library ${bsec_dir}/esp32/libalgobsec.a)

# Stop early with a hint instead of failing deep in the build.
if(NOT EXISTS ${bsec_library})
    message(FATAL_ERROR "BSEC2 submodule is missing: ${bsec_dir}\n"
        "Run: git submodule update --init")
endif()

idf_component_register(
    SRCS "sensing.c" "sensing_bsec.c" "sensing_state.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "${bsec_dir}/inc" "${bsec_dir}/config" "${bme68x_dir}"
    REQUIRES esp_driver_i2c
    PRIV_REQUIRES bme680 esp_timer log nvs_flash
)

add_prebuilt_library(algobsec ${bsec_library})
target_link_libraries(${COMPONENT_LIB} PRIVATE algobsec)
