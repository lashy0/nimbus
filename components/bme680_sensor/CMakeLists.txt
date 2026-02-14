set(srcs
    "src/bme680_sensor.c"
    "BME68x_SensorAPI/bme68x.c"
)

set(bsec2_cfg
    "Bosch-BSEC2-Library/src/config/bme680/bme680_iaq_33v_3s_4d/bsec_iaq.config"
)

set(includes
    "include"
    "BME68x_SensorAPI"
    "Bosch-BSEC2-Library/src/inc"
)

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${includes}
    REQUIRES espressif__i2c_bus nvs_flash esp_timer
    EMBED_FILES ${bsec2_cfg}
)

set(BSEC2_LIB "${CMAKE_CURRENT_LIST_DIR}/Bosch-BSEC2-Library/src/esp32/libalgobsec.a")
if(NOT EXISTS "${BSEC2_LIB}")
    message(FATAL_ERROR "BSEC2 binary not found: ${BSEC2_LIB}")
endif()

target_link_libraries(${COMPONENT_LIB} PRIVATE "${BSEC2_LIB}")
