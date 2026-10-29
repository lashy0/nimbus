idf_component_register(
    SRCS "nimbus.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES bme680 esp_app_format esp_driver_i2c log
)
