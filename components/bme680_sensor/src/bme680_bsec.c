#include "bme680_bsec.h"

#include <string.h>

#include "bme680_internal.h"
#include "bme680_nvs.h"
#include "bsec_interface.h"
#include "esp_log.h"

static const char* TAG = "bme680_bsec";

#define BSEC_CHECK_INPUT(x, shift) ((x) & (1U << ((shift) - 1U)))

extern const uint8_t bsec_iaq_config_start[] asm("_binary_bsec_iaq_config_start");
extern const uint8_t bsec_iaq_config_end[] asm("_binary_bsec_iaq_config_end");

uint32_t bme680_bsec_next_call_delay_ms(int64_t next_call_ns)
{
    int64_t delay_ns = next_call_ns - (bme680_now_us() * 1000LL);
    if (delay_ns < 0) {
        delay_ns = 0;
    }

    uint32_t delay_ms = (uint32_t)((delay_ns + 999999LL) / 1000000LL);
    if (delay_ms < 200U) {
        delay_ms = 200U;
    }
    if (delay_ms > 60000U) {
        delay_ms = 60000U;
    }

    return delay_ms;
}

esp_err_t bme680_bsec_apply_default_configuration(void)
{
    const uint8_t* config_blob = bsec_iaq_config_start;
    size_t config_len = (size_t)(bsec_iaq_config_end - bsec_iaq_config_start);
    if (config_len == 0U) {
        ESP_LOGE(TAG, "Invalid BSEC config blob length: %lu", (unsigned long)config_len);
        return ESP_FAIL;
    }

    /* Bosch .config files may embed a 32-bit LE payload length prefix. */
    if (config_len > BSEC_MAX_PROPERTY_BLOB_SIZE && config_len >= 4U) {
        uint32_t payload_len = (uint32_t)config_blob[0] | ((uint32_t)config_blob[1] << 8U) |
                               ((uint32_t)config_blob[2] << 16U) | ((uint32_t)config_blob[3] << 24U);
        if ((size_t)payload_len == (config_len - 4U) && payload_len <= BSEC_MAX_PROPERTY_BLOB_SIZE) {
            config_blob += 4U;
            config_len = payload_len;
        }
    }

    if (config_len > BSEC_MAX_PROPERTY_BLOB_SIZE) {
        ESP_LOGE(TAG, "Invalid BSEC config payload length: %lu", (unsigned long)config_len);
        return ESP_FAIL;
    }

    uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE] = {0};
    bsec_library_return_t bsec_ret =
        bsec_set_configuration(config_blob, (uint32_t)config_len, work_buffer, BSEC_MAX_WORKBUFFER_SIZE);
    return bme680_check_bsec_rslt("bsec_set_configuration", bsec_ret);
}

esp_err_t bme680_bsec_update_subscription(bme680_sensor_mode_t mode)
{
    float sample_rate = (mode == BME680_SENSOR_MODE_ULP) ? BSEC_SAMPLE_RATE_ULP : BSEC_SAMPLE_RATE_LP;

    bsec_sensor_configuration_t requested_virtual_sensors[6] = {
        {.sensor_id = BSEC_OUTPUT_IAQ, .sample_rate = sample_rate},
        {.sensor_id = BSEC_OUTPUT_STATIC_IAQ, .sample_rate = sample_rate},
        {.sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE, .sample_rate = sample_rate},
        {.sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY, .sample_rate = sample_rate},
        {.sensor_id = BSEC_OUTPUT_STABILIZATION_STATUS, .sample_rate = sample_rate},
        {.sensor_id = BSEC_OUTPUT_RUN_IN_STATUS, .sample_rate = sample_rate},
    };

    bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR] = {0};
    uint8_t n_required_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;
    bsec_library_return_t bsec_ret = bsec_update_subscription(requested_virtual_sensors,
        (uint8_t)(sizeof(requested_virtual_sensors) / sizeof(requested_virtual_sensors[0])),
        required_sensor_settings,
        &n_required_sensor_settings);
    return bme680_check_bsec_rslt("bsec_update_subscription", bsec_ret);
}

esp_err_t bme680_bsec_apply_settings(const bsec_bme_settings_t* settings)
{
    if (settings->op_mode == BME68X_SLEEP_MODE) {
        int8_t rslt = bme68x_set_op_mode(BME68X_SLEEP_MODE, &s_ctx.bme);
        if (bme680_check_bme_rslt("bme68x_set_op_mode(sleep)", rslt) != ESP_OK) {
            return ESP_FAIL;
        }
        s_ctx.current_op_mode = BME68X_SLEEP_MODE;
        return ESP_OK;
    }

    s_ctx.conf.filter = BME68X_FILTER_OFF;
    s_ctx.conf.odr = BME68X_ODR_NONE;
    s_ctx.conf.os_temp = settings->temperature_oversampling;
    s_ctx.conf.os_pres = settings->pressure_oversampling;
    s_ctx.conf.os_hum = settings->humidity_oversampling;

    int8_t rslt = bme68x_set_conf(&s_ctx.conf, &s_ctx.bme);
    if (bme680_check_bme_rslt("bme68x_set_conf", rslt) != ESP_OK) {
        return ESP_FAIL;
    }

    memset(&s_ctx.heatr_conf, 0, sizeof(s_ctx.heatr_conf));
    s_ctx.heatr_conf.enable = settings->run_gas ? BME68X_ENABLE : BME68X_DISABLE;

    if (settings->op_mode == BME68X_PARALLEL_MODE) {
        uint8_t profile_len = settings->heater_profile_len;
        if (profile_len > BSEC_HEATR_PROFILE_LEN) {
            profile_len = BSEC_HEATR_PROFILE_LEN;
        }

        for (uint8_t i = 0; i < profile_len; i++) {
            s_ctx.heatr_temp_profile[i] = settings->heater_temperature_profile[i];
            s_ctx.heatr_dur_profile[i] = settings->heater_duration_profile[i];
        }

        uint32_t meas_dur_us = bme68x_get_meas_dur(BME68X_PARALLEL_MODE, &s_ctx.conf, &s_ctx.bme);
        uint32_t meas_dur_ms = meas_dur_us / 1000U;
        s_ctx.heatr_conf.shared_heatr_dur =
            (BSEC_TOTAL_HEAT_DUR_MS > meas_dur_ms) ? (uint16_t)(BSEC_TOTAL_HEAT_DUR_MS - meas_dur_ms) : 0U;
        s_ctx.heatr_conf.heatr_temp_prof = s_ctx.heatr_temp_profile;
        s_ctx.heatr_conf.heatr_dur_prof = s_ctx.heatr_dur_profile;
        s_ctx.heatr_conf.profile_len = profile_len;
    } else {
        s_ctx.heatr_conf.heatr_temp = settings->heater_temperature;
        s_ctx.heatr_conf.heatr_dur = settings->heater_duration;
        s_ctx.heatr_conf.heatr_temp_prof = NULL;
        s_ctx.heatr_conf.heatr_dur_prof = NULL;
        s_ctx.heatr_conf.profile_len = 0;
        s_ctx.heatr_conf.shared_heatr_dur = 0;
    }

    rslt = bme68x_set_heatr_conf(settings->op_mode, &s_ctx.heatr_conf, &s_ctx.bme);
    if (bme680_check_bme_rslt("bme68x_set_heatr_conf", rslt) != ESP_OK) {
        return ESP_FAIL;
    }

    rslt = bme68x_set_op_mode(settings->op_mode, &s_ctx.bme);
    if (bme680_check_bme_rslt("bme68x_set_op_mode", rslt) != ESP_OK) {
        return ESP_FAIL;
    }

    s_ctx.current_op_mode = settings->op_mode;
    return ESP_OK;
}

static void fill_last_raw(const struct bme68x_data* raw)
{
#ifdef BME68X_USE_FPU
    s_ctx.last_output.temperature_c = raw->temperature;
    s_ctx.last_output.pressure_pa = raw->pressure;
    s_ctx.last_output.humidity_rh = raw->humidity;
    s_ctx.last_output.gas_resistance_ohm = raw->gas_resistance;
#else
    s_ctx.last_output.temperature_c = ((float)raw->temperature) / 100.0f;
    s_ctx.last_output.pressure_pa = (float)raw->pressure;
    s_ctx.last_output.humidity_rh = ((float)raw->humidity) / 1000.0f;
    s_ctx.last_output.gas_resistance_ohm = (float)raw->gas_resistance;
#endif
}

esp_err_t bme680_bsec_process_field(int64_t timestamp_ns, uint8_t op_mode, const struct bme68x_data* raw,
    uint32_t process_data_mask)
{
    fill_last_raw(raw);

    bool gas_valid = (raw->status & BME68X_GASM_VALID_MSK) != 0;
    const struct {
        uint8_t id;
        float signal;
        bool gas_dependent;
    } specs[] = {
        {BSEC_INPUT_HEATSOURCE,   0.0f,                                                                     false},
        {BSEC_INPUT_TEMPERATURE,  s_ctx.last_output.temperature_c,                                          false},
        {BSEC_INPUT_HUMIDITY,     s_ctx.last_output.humidity_rh,                                            false},
        {BSEC_INPUT_PRESSURE,     s_ctx.last_output.pressure_pa * 0.01f,                                    false},
        {BSEC_INPUT_GASRESISTOR,  s_ctx.last_output.gas_resistance_ohm,                                     true},
        {BSEC_INPUT_PROFILE_PART, (op_mode == BME68X_FORCED_MODE) ? 0.0f : (float)raw->gas_index,           true},
    };

    bsec_input_t inputs[BSEC_MAX_PHYSICAL_SENSOR] = {0};
    uint8_t n_inputs = 0;
    for (size_t i = 0; i < sizeof(specs) / sizeof(specs[0]); i++) {
        if (!BSEC_CHECK_INPUT(process_data_mask, specs[i].id)) {
            continue;
        }
        if (specs[i].gas_dependent && !gas_valid) {
            continue;
        }
        inputs[n_inputs].sensor_id = specs[i].id;
        inputs[n_inputs].signal = specs[i].signal;
        inputs[n_inputs].time_stamp = timestamp_ns;
        n_inputs++;
    }

    if (n_inputs == 0) {
        return ESP_OK;
    }

    bsec_output_t outputs[BSEC_NUMBER_OUTPUTS] = {0};
    uint8_t n_outputs = BSEC_NUMBER_OUTPUTS;
    bsec_library_return_t bsec_ret = bsec_do_steps(inputs, n_inputs, outputs, &n_outputs);
    if (bme680_check_bsec_rslt("bsec_do_steps", bsec_ret) != ESP_OK) {
        return ESP_FAIL;
    }

    bool has_iaq_output = false;
    for (uint8_t i = 0; i < n_outputs; i++) {
        switch (outputs[i].sensor_id) {
            case BSEC_OUTPUT_IAQ:
                s_ctx.last_output.iaq = (uint16_t)(bme680_clampf(outputs[i].signal, 0.0f, 500.0f) + 0.5f);
                s_ctx.iaq_accuracy = outputs[i].accuracy;
                s_ctx.last_output.iaq_accuracy = outputs[i].accuracy;
                has_iaq_output = true;
                break;

            case BSEC_OUTPUT_STATIC_IAQ:
                s_ctx.last_output.static_iaq = (uint16_t)(bme680_clampf(outputs[i].signal, 0.0f, 500.0f) + 0.5f);
                break;

            case BSEC_OUTPUT_STABILIZATION_STATUS:
                s_ctx.last_output.stabilization_done = (outputs[i].signal >= 0.5f);
                break;

            case BSEC_OUTPUT_RUN_IN_STATUS:
                s_ctx.last_output.run_in_done = (outputs[i].signal >= 0.5f);
                break;

            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
                s_ctx.last_output.temperature_c = outputs[i].signal;
                break;

            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
                s_ctx.last_output.humidity_rh = outputs[i].signal;
                break;

            default:
                break;
        }
    }

    if (has_iaq_output) {
        s_ctx.iaq_valid = (s_ctx.iaq_accuracy > 0U);
        bme680_nvs_save_state_on_progress();
    }

    s_ctx.last_output.iaq_valid = s_ctx.iaq_valid;
    return ESP_OK;
}

uint32_t bme680_bsec_measurement_wait_us(const bsec_bme_settings_t* settings)
{
    uint32_t meas_dur_us = bme68x_get_meas_dur(settings->op_mode, &s_ctx.conf, &s_ctx.bme);

    if (settings->op_mode == BME68X_FORCED_MODE) {
        meas_dur_us += ((uint32_t)settings->heater_duration * 1000U);
    } else if (settings->op_mode == BME68X_PARALLEL_MODE) {
        meas_dur_us += ((uint32_t)BSEC_TOTAL_HEAT_DUR_MS * 1000U);
    }

    return meas_dur_us + 1000U;
}
