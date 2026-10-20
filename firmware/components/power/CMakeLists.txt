idf_component_register(
    SRCS "power.c" "power_battery.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_hw_support log
)
