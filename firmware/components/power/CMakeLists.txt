idf_component_register(
    SRCS
        "power.c"
        "power_battery.c"
        "power_estimate.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES
        esp_adc
        esp_driver_gpio
        log
)
