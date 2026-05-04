#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "bme68x.h"
#include "bme680_sensor.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "i2c_bus.h"

#define BSEC_HEATR_PROFILE_LEN 10U
#define BSEC_DEFAULT_NEXT_CALL_DELAY_MS 3000U
#define BSEC_TOTAL_HEAT_DUR_MS 140U

typedef struct {
    bool initialized;
    bool iaq_valid;
    uint8_t iaq_accuracy;
    uint8_t current_op_mode;
    bool state_persistence_enabled;
    uint32_t next_call_delay_ms;
    uint8_t last_saved_iaq_accuracy;
    bool last_saved_stabilization_done;
    bool last_saved_run_in_done;
    bme680_sensor_mode_t mode;
    int64_t last_state_save_time_us;

    i2c_bus_handle_t bus;
    i2c_bus_device_handle_t dev_handle;
    gpio_num_t sda_gpio;
    gpio_num_t scl_gpio;
    i2c_port_t i2c_port;
    uint32_t i2c_clk_speed_hz;

    struct bme68x_dev bme;
    struct bme68x_conf conf;
    struct bme68x_heatr_conf heatr_conf;
    uint16_t heatr_temp_profile[BSEC_HEATR_PROFILE_LEN];
    uint16_t heatr_dur_profile[BSEC_HEATR_PROFILE_LEN];

    bme680_sensor_data_t last_output;
} bme680_sensor_ctx_t;

extern bme680_sensor_ctx_t s_ctx;

int64_t bme680_now_us(void);
float bme680_clampf(float value, float min_v, float max_v);
esp_err_t bme680_check_bme_rslt(const char* step, int8_t rslt);
esp_err_t bme680_check_bsec_rslt(const char* step, int rslt);
