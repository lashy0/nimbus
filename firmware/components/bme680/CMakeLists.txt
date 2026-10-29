set(bme68x_dir ${CMAKE_CURRENT_LIST_DIR}/../../3rdparty/bme68x)

# Stop early with a hint instead of failing deep in the build.
if(NOT EXISTS ${bme68x_dir}/bme68x.c)
    message(FATAL_ERROR "BME68x SensorAPI submodule is missing: ${bme68x_dir}\n"
        "Run: git submodule update --init")
endif()

idf_component_register(
    SRCS "bme680.c" "${bme68x_dir}/bme68x.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "${bme68x_dir}"
    REQUIRES esp_driver_i2c
    PRIV_REQUIRES log
)
