#include "bme680_sensor.h"

#include <string.h>

#include "bme68x.h"
#include "bsec_interface.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#define BSEC_CHECK_INPUT(x, shift) ((x) & (1U << ((shift) - 1U)))

#define BSEC_STATE_SAVE_INTERVAL_SEC (15U * 60U)
#define BSEC_STATE_SAVE_INTERVAL_BOOTSTRAP_SEC 60U
#define BSEC_DEFAULT_NEXT_CALL_DELAY_MS 3000U
#define BSEC_TOTAL_HEAT_DUR_MS 140U
#define BSEC_HEATR_PROFILE_LEN 10U

#define BSEC_NVS_NAMESPACE "bme680"
#define BSEC_NVS_KEY_STATE "bsec_state"
#define BSEC_NVS_KEY_STATE_LEN "bsec_len"

extern const uint8_t bsec_iaq_config_start[] asm("_binary_bsec_iaq_config_start");
extern const uint8_t bsec_iaq_config_end[] asm("_binary_bsec_iaq_config_end");

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

static const char* TAG = "bme680_sensor";
static bme680_sensor_ctx_t s_ctx;

static void i2c_bus_recovery(gpio_num_t sda_gpio, gpio_num_t scl_gpio);

static int64_t now_us(void)
{
    return esp_timer_get_time();
}

static float clampf(float value, float min_v, float max_v)
{
    if (value < min_v) {
        return min_v;
    }

    if (value > max_v) {
        return max_v;
    }

    return value;
}

static uint32_t bsec_next_call_delay_ms(int64_t next_call_ns)
{
    int64_t delay_ns = next_call_ns - (now_us() * 1000LL);
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

static BME68X_INTF_RET_TYPE bme_i2c_read(uint8_t reg_addr, uint8_t* reg_data, uint32_t length, void* intf_ptr)
{
    i2c_bus_device_handle_t dev = (i2c_bus_device_handle_t)intf_ptr;
    if (!dev || !reg_data || length == 0U) {
        return BME68X_E_COM_FAIL;
    }

    esp_err_t ret = i2c_bus_read_bytes(dev, reg_addr, length, reg_data);
    if (ret != ESP_OK && s_ctx.initialized) {
        i2c_bus_recovery(s_ctx.sda_gpio, s_ctx.scl_gpio);
        ret = i2c_bus_read_bytes(dev, reg_addr, length, reg_data);
    }
    return (ret == ESP_OK) ? BME68X_INTF_RET_SUCCESS : BME68X_E_COM_FAIL;
}

static BME68X_INTF_RET_TYPE bme_i2c_write(uint8_t reg_addr, const uint8_t* reg_data, uint32_t length, void* intf_ptr)
{
    i2c_bus_device_handle_t dev = (i2c_bus_device_handle_t)intf_ptr;
    if (!dev || !reg_data || length == 0U) {
        return BME68X_E_COM_FAIL;
    }

    esp_err_t ret = i2c_bus_write_bytes(dev, reg_addr, length, reg_data);
    return (ret == ESP_OK) ? BME68X_INTF_RET_SUCCESS : BME68X_E_COM_FAIL;
}

static void bme_delay_us(uint32_t period, void* intf_ptr)
{
    (void)intf_ptr;
    esp_rom_delay_us(period);
}

static esp_err_t bme_check_rslt(const char* step, int8_t rslt)
{
    if (rslt < BME68X_OK) {
        ESP_LOGE(TAG, "%s failed: %d", step, rslt);
        return ESP_FAIL;
    }

    if (rslt > BME68X_OK) {
        ESP_LOGW(TAG, "%s warning: %d", step, rslt);
    }

    return ESP_OK;
}

static esp_err_t bsec_check_rslt(const char* step, bsec_library_return_t rslt)
{
    if (rslt < BSEC_OK) {
        ESP_LOGE(TAG, "%s failed: %d", step, (int)rslt);
        return ESP_FAIL;
    }

    if (rslt > BSEC_OK) {
        ESP_LOGW(TAG, "%s warning: %d", step, (int)rslt);
    }

    return ESP_OK;
}

static esp_err_t bsec_apply_default_configuration(void)
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
    return bsec_check_rslt("bsec_set_configuration", bsec_ret);
}

static esp_err_t bsec_update_subscription_for_mode(bme680_sensor_mode_t mode)
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
    return bsec_check_rslt("bsec_update_subscription", bsec_ret);
}

static void bsec_try_init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_OK) {
        return;
    }

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS requires erase, retrying");
        if (nvs_flash_erase() == ESP_OK) {
            ret = nvs_flash_init();
            if (ret == ESP_OK) {
                return;
            }
        }
    }

    ESP_LOGW(TAG, "NVS init failed: %s", esp_err_to_name(ret));
}

static void bsec_clear_state_nvs(void)
{
    if (!s_ctx.state_persistence_enabled) {
        return;
    }

    nvs_handle_t nvs = 0;
    if (nvs_open(BSEC_NVS_NAMESPACE, NVS_READWRITE, &nvs) != ESP_OK) {
        return;
    }

    nvs_erase_key(nvs, BSEC_NVS_KEY_STATE);
    nvs_erase_key(nvs, BSEC_NVS_KEY_STATE_LEN);
    nvs_commit(nvs);
    nvs_close(nvs);
}

static void bsec_load_state_nvs(void)
{
    if (!s_ctx.state_persistence_enabled) {
        return;
    }

    nvs_handle_t nvs = 0;
    if (nvs_open(BSEC_NVS_NAMESPACE, NVS_READONLY, &nvs) != ESP_OK) {
        return;
    }

    uint32_t state_len = 0;
    esp_err_t ret = nvs_get_u32(nvs, BSEC_NVS_KEY_STATE_LEN, &state_len);
    if (ret != ESP_OK || state_len == 0U || state_len > BSEC_MAX_STATE_BLOB_SIZE) {
        nvs_close(nvs);
        return;
    }

    uint8_t state_blob[BSEC_MAX_STATE_BLOB_SIZE] = {0};
    size_t blob_size = state_len;
    ret = nvs_get_blob(nvs, BSEC_NVS_KEY_STATE, state_blob, &blob_size);
    nvs_close(nvs);
    if (ret != ESP_OK || blob_size != state_len) {
        return;
    }

    uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE] = {0};
    bsec_library_return_t bsec_ret = bsec_set_state(state_blob, state_len, work_buffer, BSEC_MAX_WORKBUFFER_SIZE);
    if (bsec_check_rslt("bsec_set_state", bsec_ret) == ESP_OK) {
        ESP_LOGI(TAG, "Loaded BSEC state from NVS (%lu bytes)", (unsigned long)state_len);
    }
}

static void bsec_save_state_nvs(bool force)
{
    if (!s_ctx.state_persistence_enabled || !s_ctx.initialized) {
        return;
    }

    int64_t now = now_us();
    bool calibration_ready =
        (s_ctx.iaq_accuracy >= 2U) && s_ctx.last_output.stabilization_done && s_ctx.last_output.run_in_done;
    uint32_t interval_sec = calibration_ready ? BSEC_STATE_SAVE_INTERVAL_SEC : BSEC_STATE_SAVE_INTERVAL_BOOTSTRAP_SEC;
    int64_t interval_us = (int64_t)interval_sec * 1000000LL;
    if (!force && s_ctx.last_state_save_time_us != 0 && (now - s_ctx.last_state_save_time_us) < interval_us) {
        return;
    }

    uint8_t state_blob[BSEC_MAX_STATE_BLOB_SIZE] = {0};
    uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE] = {0};
    uint32_t state_len = BSEC_MAX_STATE_BLOB_SIZE;

    bsec_library_return_t bsec_ret =
        bsec_get_state(0, state_blob, BSEC_MAX_STATE_BLOB_SIZE, work_buffer, BSEC_MAX_WORKBUFFER_SIZE, &state_len);
    if (bsec_check_rslt("bsec_get_state", bsec_ret) != ESP_OK) {
        return;
    }

    nvs_handle_t nvs = 0;
    esp_err_t ret = nvs_open(BSEC_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to open NVS for BSEC state save: %s", esp_err_to_name(ret));
        return;
    }

    ret = nvs_set_blob(nvs, BSEC_NVS_KEY_STATE, state_blob, state_len);
    if (ret == ESP_OK) {
        ret = nvs_set_u32(nvs, BSEC_NVS_KEY_STATE_LEN, state_len);
    }
    if (ret == ESP_OK) {
        ret = nvs_commit(nvs);
    }

    if (ret == ESP_OK) {
        s_ctx.last_state_save_time_us = now;
    } else {
        ESP_LOGW(TAG, "Failed to persist BSEC state: %s", esp_err_to_name(ret));
    }

    nvs_close(nvs);
}

static void bsec_maybe_save_state_on_progress(void)
{
    if (!s_ctx.state_persistence_enabled || !s_ctx.initialized) {
        return;
    }

    bool has_progress = (s_ctx.iaq_accuracy > s_ctx.last_saved_iaq_accuracy) ||
                        (s_ctx.last_output.stabilization_done && !s_ctx.last_saved_stabilization_done) ||
                        (s_ctx.last_output.run_in_done && !s_ctx.last_saved_run_in_done);
    if (!has_progress) {
        return;
    }

    bsec_save_state_nvs(true);
    s_ctx.last_saved_iaq_accuracy = s_ctx.iaq_accuracy;
    s_ctx.last_saved_stabilization_done = s_ctx.last_output.stabilization_done;
    s_ctx.last_saved_run_in_done = s_ctx.last_output.run_in_done;
}

static esp_err_t bme_apply_settings(const bsec_bme_settings_t* settings)
{
    if (settings->op_mode == BME68X_SLEEP_MODE) {
        int8_t rslt = bme68x_set_op_mode(BME68X_SLEEP_MODE, &s_ctx.bme);
        if (bme_check_rslt("bme68x_set_op_mode(sleep)", rslt) != ESP_OK) {
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
    if (bme_check_rslt("bme68x_set_conf", rslt) != ESP_OK) {
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
    if (bme_check_rslt("bme68x_set_heatr_conf", rslt) != ESP_OK) {
        return ESP_FAIL;
    }

    rslt = bme68x_set_op_mode(settings->op_mode, &s_ctx.bme);
    if (bme_check_rslt("bme68x_set_op_mode", rslt) != ESP_OK) {
        return ESP_FAIL;
    }

    s_ctx.current_op_mode = settings->op_mode;
    return ESP_OK;
}

static void bme_fill_last_raw(const struct bme68x_data* raw)
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

static esp_err_t bme_process_field(
    int64_t timestamp_ns, uint8_t op_mode, const struct bme68x_data* raw, uint32_t process_data_mask)
{
    bme_fill_last_raw(raw);

    bsec_input_t inputs[BSEC_MAX_PHYSICAL_SENSOR] = {0};
    uint8_t n_inputs = 0;

    if (BSEC_CHECK_INPUT(process_data_mask, BSEC_INPUT_HEATSOURCE)) {
        inputs[n_inputs].sensor_id = BSEC_INPUT_HEATSOURCE;
        inputs[n_inputs].signal = 0.0f;
        inputs[n_inputs].time_stamp = timestamp_ns;
        n_inputs++;
    }

    if (BSEC_CHECK_INPUT(process_data_mask, BSEC_INPUT_TEMPERATURE)) {
        inputs[n_inputs].sensor_id = BSEC_INPUT_TEMPERATURE;
        inputs[n_inputs].signal = s_ctx.last_output.temperature_c;
        inputs[n_inputs].time_stamp = timestamp_ns;
        n_inputs++;
    }

    if (BSEC_CHECK_INPUT(process_data_mask, BSEC_INPUT_HUMIDITY)) {
        inputs[n_inputs].sensor_id = BSEC_INPUT_HUMIDITY;
        inputs[n_inputs].signal = s_ctx.last_output.humidity_rh;
        inputs[n_inputs].time_stamp = timestamp_ns;
        n_inputs++;
    }

    if (BSEC_CHECK_INPUT(process_data_mask, BSEC_INPUT_PRESSURE)) {
        inputs[n_inputs].sensor_id = BSEC_INPUT_PRESSURE;
        inputs[n_inputs].signal = s_ctx.last_output.pressure_pa * 0.01f;
        inputs[n_inputs].time_stamp = timestamp_ns;
        n_inputs++;
    }

    if (BSEC_CHECK_INPUT(process_data_mask, BSEC_INPUT_GASRESISTOR) && (raw->status & BME68X_GASM_VALID_MSK)) {
        inputs[n_inputs].sensor_id = BSEC_INPUT_GASRESISTOR;
        inputs[n_inputs].signal = s_ctx.last_output.gas_resistance_ohm;
        inputs[n_inputs].time_stamp = timestamp_ns;
        n_inputs++;
    }

    if (BSEC_CHECK_INPUT(process_data_mask, BSEC_INPUT_PROFILE_PART) && (raw->status & BME68X_GASM_VALID_MSK)) {
        inputs[n_inputs].sensor_id = BSEC_INPUT_PROFILE_PART;
        inputs[n_inputs].signal = (op_mode == BME68X_FORCED_MODE) ? 0.0f : (float)raw->gas_index;
        inputs[n_inputs].time_stamp = timestamp_ns;
        n_inputs++;
    }

    if (n_inputs == 0) {
        return ESP_OK;
    }

    bsec_output_t outputs[BSEC_NUMBER_OUTPUTS] = {0};
    uint8_t n_outputs = BSEC_NUMBER_OUTPUTS;
    bsec_library_return_t bsec_ret = bsec_do_steps(inputs, n_inputs, outputs, &n_outputs);
    if (bsec_check_rslt("bsec_do_steps", bsec_ret) != ESP_OK) {
        return ESP_FAIL;
    }

    bool has_iaq_output = false;
    for (uint8_t i = 0; i < n_outputs; i++) {
        switch (outputs[i].sensor_id) {
            case BSEC_OUTPUT_IAQ:
                s_ctx.last_output.iaq = (uint16_t)(clampf(outputs[i].signal, 0.0f, 500.0f) + 0.5f);
                s_ctx.iaq_accuracy = outputs[i].accuracy;
                s_ctx.last_output.iaq_accuracy = outputs[i].accuracy;
                has_iaq_output = true;
                break;

            case BSEC_OUTPUT_STATIC_IAQ:
                s_ctx.last_output.static_iaq = (uint16_t)(clampf(outputs[i].signal, 0.0f, 500.0f) + 0.5f);
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
        bsec_maybe_save_state_on_progress();
    }

    s_ctx.last_output.iaq_valid = s_ctx.iaq_valid;
    return ESP_OK;
}

static uint32_t bme_measurement_wait_us(const bsec_bme_settings_t* settings)
{
    uint32_t meas_dur_us = bme68x_get_meas_dur(settings->op_mode, &s_ctx.conf, &s_ctx.bme);

    if (settings->op_mode == BME68X_FORCED_MODE) {
        meas_dur_us += ((uint32_t)settings->heater_duration * 1000U);
    } else if (settings->op_mode == BME68X_PARALLEL_MODE) {
        meas_dur_us += ((uint32_t)BSEC_TOTAL_HEAT_DUR_MS * 1000U);
    }

    return meas_dur_us + 1000U;
}

static void i2c_bus_recovery(gpio_num_t sda_gpio, gpio_num_t scl_gpio)
{
    int sda_level = gpio_get_level(sda_gpio);
    if (sda_level != 0) {
        return;
    }

    ESP_LOGW(TAG, "SDA stuck low, attempting I2C bus recovery");

    gpio_config_t scl_cfg = {
        .pin_bit_mask = (1ULL << scl_gpio),
        .mode = GPIO_MODE_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&scl_cfg);

    for (int i = 0; i < 9; i++) {
        gpio_set_level(scl_gpio, 0);
        esp_rom_delay_us(5);
        gpio_set_level(scl_gpio, 1);
        esp_rom_delay_us(5);
        if (gpio_get_level(sda_gpio) != 0) {
            break;
        }
    }

    if (gpio_get_level(sda_gpio) != 0) {
        ESP_LOGI(TAG, "I2C bus recovery successful");
    } else {
        ESP_LOGE(TAG, "I2C bus recovery failed, SDA still low");
    }
}

esp_err_t bme680_sensor_init(const bme680_sensor_config_t* config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.current_op_mode = BME68X_SLEEP_MODE;
    s_ctx.state_persistence_enabled = !config->disable_state_persistence;
    s_ctx.next_call_delay_ms = BSEC_DEFAULT_NEXT_CALL_DELAY_MS;
    s_ctx.mode = BME680_SENSOR_MODE_LP;
    esp_reset_reason_t reset_reason = esp_reset_reason();
    bool cold_start_on_power_on = config->reset_baseline_on_power_on && (reset_reason == ESP_RST_POWERON);

    ESP_LOGI(TAG,
        "System reset reason=%d, BSEC baseline reset on power-on=%s",
        (int)reset_reason,
        cold_start_on_power_on ? "yes" : "no");

    bsec_try_init_nvs();

    s_ctx.sda_gpio = config->sda_io_num;
    s_ctx.scl_gpio = config->scl_io_num;
    s_ctx.i2c_port = config->i2c_port;
    s_ctx.i2c_clk_speed_hz = config->i2c_clk_speed_hz;

    i2c_bus_recovery(config->sda_io_num, config->scl_io_num);

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = config->sda_io_num,
        .scl_io_num = config->scl_io_num,
        .sda_pullup_en = true,
        .scl_pullup_en = true,
        .master.clk_speed = config->i2c_clk_speed_hz,
        .clk_flags = 0,
    };

    s_ctx.bus = i2c_bus_create(config->i2c_port, &i2c_conf);
    if (!s_ctx.bus) {
        ESP_LOGE(TAG, "Failed to create i2c bus");
        return ESP_FAIL;
    }

    s_ctx.dev_handle = i2c_bus_device_create(s_ctx.bus, config->i2c_addr, config->i2c_clk_speed_hz);
    if (!s_ctx.dev_handle) {
        ESP_LOGE(TAG, "Failed to create bme680 i2c device");
        bme680_sensor_deinit();
        return ESP_FAIL;
    }

    s_ctx.bme.intf = BME68X_I2C_INTF;
    s_ctx.bme.intf_ptr = s_ctx.dev_handle;
    s_ctx.bme.read = bme_i2c_read;
    s_ctx.bme.write = bme_i2c_write;
    s_ctx.bme.delay_us = bme_delay_us;
    s_ctx.bme.amb_temp = 25;

    int8_t rslt = bme68x_init(&s_ctx.bme);
    if (bme_check_rslt("bme68x_init", rslt) != ESP_OK) {
        bme680_sensor_deinit();
        return ESP_FAIL;
    }

    bsec_library_return_t bsec_ret = bsec_init();
    if (bsec_check_rslt("bsec_init", bsec_ret) != ESP_OK) {
        bme680_sensor_deinit();
        return ESP_FAIL;
    }

    if (bsec_apply_default_configuration() != ESP_OK) {
        bme680_sensor_deinit();
        return ESP_FAIL;
    }

    bsec_version_t version = {0};
    bsec_ret = bsec_get_version(&version);
    if (bsec_check_rslt("bsec_get_version", bsec_ret) == ESP_OK) {
        ESP_LOGI(
            TAG, "BSEC version %u.%u.%u.%u", version.major, version.minor, version.major_bugfix, version.minor_bugfix);
    }

    if (bsec_update_subscription_for_mode(BME680_SENSOR_MODE_LP) != ESP_OK) {
        bme680_sensor_deinit();
        return ESP_FAIL;
    }

    if (cold_start_on_power_on) {
        bsec_clear_state_nvs();
        ESP_LOGI(TAG, "BSEC cold-start baseline: power-on reset, persisted state cleared");
    } else {
        bsec_load_state_nvs();
    }

    s_ctx.initialized = true;
    return ESP_OK;
}

esp_err_t bme680_sensor_read(bme680_sensor_data_t* out_data)
{
    if (!s_ctx.initialized || !out_data) {
        return ESP_ERR_INVALID_STATE;
    }

    int64_t timestamp_ns = now_us() * 1000LL;
    bsec_bme_settings_t bme_settings = {0};
    bsec_library_return_t bsec_ret = bsec_sensor_control(timestamp_ns, &bme_settings);
    if (bsec_check_rslt("bsec_sensor_control", bsec_ret) != ESP_OK) {
        return ESP_FAIL;
    }

    s_ctx.next_call_delay_ms = bsec_next_call_delay_ms(bme_settings.next_call);

    if ((bme_settings.trigger_measurement != 0U) || (bme_settings.op_mode != s_ctx.current_op_mode)) {
        if (bme_apply_settings(&bme_settings) != ESP_OK) {
            return ESP_FAIL;
        }
    }

    if (bme_settings.trigger_measurement != 0U && bme_settings.op_mode != BME68X_SLEEP_MODE) {
        uint32_t wait_us = bme_measurement_wait_us(&bme_settings);
        uint32_t wait_ms = (wait_us + 999U) / 1000U + 5U;
        if (wait_ms > 0U) {
            vTaskDelay(pdMS_TO_TICKS(wait_ms));
        }

        struct bme68x_data fields[BME68X_N_MEAS] = {0};
        uint8_t n_fields = 0;
        int8_t rslt = bme68x_get_data(bme_settings.op_mode, fields, &n_fields, &s_ctx.bme);
        if (bme_check_rslt("bme68x_get_data", rslt) != ESP_OK || n_fields == 0U) {
            return ESP_FAIL;
        }

        for (uint8_t i = 0; i < n_fields; i++) {
            if (bme_process_field(timestamp_ns, bme_settings.op_mode, &fields[i], bme_settings.process_data) !=
                ESP_OK) {
                return ESP_FAIL;
            }
        }
    }

    bsec_save_state_nvs(false);
    *out_data = s_ctx.last_output;
    return ESP_OK;
}

esp_err_t bme680_sensor_set_mode(bme680_sensor_mode_t mode)
{
    if (!s_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (mode != BME680_SENSOR_MODE_LP && mode != BME680_SENSOR_MODE_ULP) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_ctx.mode == mode) {
        return ESP_OK;
    }

    if (bsec_update_subscription_for_mode(mode) != ESP_OK) {
        return ESP_FAIL;
    }

    s_ctx.mode = mode;
    s_ctx.next_call_delay_ms = BSEC_DEFAULT_NEXT_CALL_DELAY_MS;
    ESP_LOGI(TAG, "BSEC mode switched to %s", mode == BME680_SENSOR_MODE_ULP ? "ULP" : "LP");
    return ESP_OK;
}

void bme680_sensor_deinit(void)
{
    if (s_ctx.initialized) {
        bsec_save_state_nvs(true);
    }

    if (s_ctx.dev_handle) {
        int8_t rslt = bme68x_set_op_mode(BME68X_SLEEP_MODE, &s_ctx.bme);
        if (rslt != BME68X_OK) {
            ESP_LOGW(TAG, "bme68x_set_op_mode(sleep) failed during deinit: %d", rslt);
        }
    }

    if (s_ctx.dev_handle) {
        i2c_bus_device_delete(&s_ctx.dev_handle);
    }

    if (s_ctx.bus) {
        i2c_bus_delete(&s_ctx.bus);
    }

    memset(&s_ctx, 0, sizeof(s_ctx));
}

bool bme680_sensor_is_initialized(void)
{
    return s_ctx.initialized;
}

bool bme680_sensor_is_calibrating(void)
{
    return s_ctx.initialized && !s_ctx.iaq_valid;
}

uint32_t bme680_sensor_get_next_call_delay_ms(void)
{
    if (!s_ctx.initialized) {
        return BSEC_DEFAULT_NEXT_CALL_DELAY_MS;
    }

    return s_ctx.next_call_delay_ms;
}
