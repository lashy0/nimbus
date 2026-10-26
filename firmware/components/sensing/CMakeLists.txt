set(third_party ${CMAKE_CURRENT_LIST_DIR}/../../third_party)
set(bsec_dir ${third_party}/bsec2/src)
set(bme68x_dir ${third_party}/bme68x)
set(bsec_library ${bsec_dir}/esp32/libalgobsec.a)
set(bsec_config ${bsec_dir}/config/bme680/bme680_iaq_33v_3s_4d/bsec_iaq.config)

# Sensing is mandatory: stop early with a hint instead of failing deep in the build.
set(missing "")
foreach(file ${bsec_library} ${bsec_config} ${bsec_dir}/inc/bsec_interface_multi.h ${bme68x_dir}/bme68x.c)
    if(NOT EXISTS ${file})
        list(APPEND missing ${file})
    endif()
endforeach()
if(missing)
    list(JOIN missing "\n  " missing_list)
    message(FATAL_ERROR "BSEC2/BME68x submodules are missing:\n  ${missing_list}\n"
        "Run: git submodule update --init")
endif()

# BSEC 2.6.1.0 at the pinned submodule revision.
set(expected_hashes
    "${bsec_library}=571f40cfeb56971bbec8bd432553d88c4ad58f164d7ee41f4870c80fb08448ca"
    "${bsec_config}=bc1cdef169aa28b99ffda85ed0a7c9b08ec0d7f6c1fe2edf4cb298a379db242b")
foreach(entry ${expected_hashes})
    string(REPLACE "=" ";" parts ${entry})
    list(GET parts 0 file)
    list(GET parts 1 expected)
    file(SHA256 ${file} actual)
    if(NOT actual STREQUAL expected)
        message(FATAL_ERROR "${file}: SHA-256 ${actual}, expected ${expected}. "
            "Check out the pinned submodule revision.")
    endif()
endforeach()

idf_component_register(
    SRCS "sensing.c" "sensing_bme68x.c" "sensing_bsec.c" "sensing_policy.c" "sensing_state.c" "${bme68x_dir}/bme68x.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "${bsec_dir}/inc" "${bme68x_dir}"
    REQUIRES esp_driver_i2c
    PRIV_REQUIRES esp_rom esp_timer log storage
    EMBED_FILES "${bsec_config}"
)

add_prebuilt_library(algobsec ${bsec_library})
target_link_libraries(${COMPONENT_LIB} PRIVATE algobsec)
