set(nimbus_sources "app_main.c")
if(CONFIG_NIMBUS_BOARD_DIAGNOSTICS)
    list(APPEND nimbus_sources "board_diagnostics.c")
endif()

# Requirements are collected before Kconfig; keep them unconditional.
idf_component_register(
    SRCS ${nimbus_sources}
    PRIV_REQUIRES esp_system log espcoredump board esp_driver_uart esp_hw_support
)
