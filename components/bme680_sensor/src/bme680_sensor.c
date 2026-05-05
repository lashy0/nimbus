#include "bme680_sensor.h"

#include <string.h>

#include "bme68x.h"
#include "bme680_bsec.h"
#include "bme680_bus.h"
#include "bme680_internal.h"
#include "bme680_nvs.h"
#include "bsec_interface.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "bme680_sensor";

bme680_sensor_ctx_t s_ctx;

static bme680_sensor_config_t s_last_config;
static bool s_has_last_config = false;

int64_t bme680_now_us(void)
{
    return esp_timer_get_time();
}

float bme680_clampf(float value, float min_v, float max_v)
{
    if (value < min_v) {
        return min_v;
    }
    if (value > max_v) {
        return max_v;
    }
    return value;
}

esp_err_t bme680_check_bme_rslt(const char* step, int8_t rslt)
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

esp_err_t bme680_check_bsec_rslt(const char* step, int rslt)
{
    if (rslt < BSEC_OK) {
        ESP_LOGE(TAG, "%s failed: %d", step, rslt);
        return ESP_FAIL;
    }
    if (rslt > BSEC_OK) {
        ESP_LOGW(TAG, "%s warning: %d", step, rslt);
    }
    return ESP_OK;
}

esp_err_t bme680_sensor_init(const bme680_sensor_config_t* config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    s_last_config = *config;
    s_has_last_config = true;

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

    s_ctx.sda_gpio = config->sda_io_num;
    s_ctx.scl_gpio = config->scl_io_num;
    s_ctx.i2c_port = config->i2c_port;
    s_ctx.i2c_clk_speed_hz = config->i2c_clk_speed_hz;

    bme680_bus_recovery(config->sda_io_num, config->scl_io_num);

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
    s_ctx.bme.read = bme680_bus_read;
    s_ctx.bme.write = bme680_bus_write;
    s_ctx.bme.delay_us = bme680_bus_delay_us;
    s_ctx.bme.amb_temp = 25;

    int8_t rslt = bme68x_init(&s_ctx.bme);
    if (bme680_check_bme_rslt("bme68x_init", rslt) != ESP_OK) {
        bme680_sensor_deinit();
        return ESP_FAIL;
    }

    bsec_library_return_t bsec_ret = bsec_init();
    if (bme680_check_bsec_rslt("bsec_init", bsec_ret) != ESP_OK) {
        bme680_sensor_deinit();
        return ESP_FAIL;
    }

    if (bme680_bsec_apply_default_configuration() != ESP_OK) {
        bme680_sensor_deinit();
        return ESP_FAIL;
    }

    bsec_version_t version = {0};
    bsec_ret = bsec_get_version(&version);
    if (bme680_check_bsec_rslt("bsec_get_version", bsec_ret) == ESP_OK) {
        ESP_LOGI(
            TAG, "BSEC version %u.%u.%u.%u", version.major, version.minor, version.major_bugfix, version.minor_bugfix);
    }

    if (bme680_bsec_update_subscription(BME680_SENSOR_MODE_LP) != ESP_OK) {
        bme680_sensor_deinit();
        return ESP_FAIL;
    }

    if (cold_start_on_power_on) {
        bme680_nvs_clear_state();
        ESP_LOGI(TAG, "BSEC cold-start baseline: power-on reset, persisted state cleared");
    } else {
        bme680_nvs_load_state();
    }

    s_ctx.initialized = true;
    return ESP_OK;
}

esp_err_t bme680_sensor_read(bme680_sensor_data_t* out_data)
{
    if (!s_ctx.initialized || !out_data) {
        return ESP_ERR_INVALID_STATE;
    }

    int64_t timestamp_ns = bme680_now_us() * 1000LL;
    bsec_bme_settings_t bme_settings = {0};
    bsec_library_return_t bsec_ret = bsec_sensor_control(timestamp_ns, &bme_settings);
    if (bme680_check_bsec_rslt("bsec_sensor_control", bsec_ret) != ESP_OK) {
        return ESP_FAIL;
    }

    s_ctx.next_call_delay_ms = bme680_bsec_next_call_delay_ms(bme_settings.next_call);

    if ((bme_settings.trigger_measurement != 0U) || (bme_settings.op_mode != s_ctx.current_op_mode)) {
        if (bme680_bsec_apply_settings(&bme_settings) != ESP_OK) {
            return ESP_FAIL;
        }
    }

    if (bme_settings.trigger_measurement != 0U && bme_settings.op_mode != BME68X_SLEEP_MODE) {
        uint32_t wait_us = bme680_bsec_measurement_wait_us(&bme_settings);
        uint32_t wait_ms = (wait_us + 999U) / 1000U + 5U;
        if (wait_ms > 0U) {
            vTaskDelay(pdMS_TO_TICKS(wait_ms));
        }

        struct bme68x_data fields[BME68X_N_MEAS] = {0};
        uint8_t n_fields = 0;
        int8_t rslt = bme68x_get_data(bme_settings.op_mode, fields, &n_fields, &s_ctx.bme);
        if (bme680_check_bme_rslt("bme68x_get_data", rslt) != ESP_OK || n_fields == 0U) {
            return ESP_FAIL;
        }

        for (uint8_t i = 0; i < n_fields; i++) {
            if (bme680_bsec_process_field(timestamp_ns, bme_settings.op_mode, &fields[i], bme_settings.process_data) !=
                ESP_OK) {
                return ESP_FAIL;
            }
        }
    }

    bme680_nvs_save_state(false);
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

    if (bme680_bsec_update_subscription(mode) != ESP_OK) {
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
        bme680_nvs_save_state(true);
    }

    if (s_ctx.dev_handle) {
        int8_t rslt = bme68x_set_op_mode(BME68X_SLEEP_MODE, &s_ctx.bme);
        if (rslt != BME68X_OK) {
            ESP_LOGW(TAG, "bme68x_set_op_mode(sleep) failed during deinit: %d", rslt);
        }
        i2c_bus_device_delete(&s_ctx.dev_handle);
    }

    if (s_ctx.bus) {
        i2c_bus_delete(&s_ctx.bus);
    }

    memset(&s_ctx, 0, sizeof(s_ctx));
}

esp_err_t bme680_sensor_reinit(void)
{
    if (!s_has_last_config) {
        return ESP_ERR_INVALID_STATE;
    }
    bme680_sensor_config_t saved = s_last_config;
    bme680_sensor_deinit();
    return bme680_sensor_init(&saved);
}

bool bme680_sensor_probe(const bme680_sensor_config_t* config)
{
    if (!config) {
        return false;
    }

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = config->sda_io_num,
        .scl_io_num = config->scl_io_num,
        .sda_pullup_en = true,
        .scl_pullup_en = true,
        .master.clk_speed = config->i2c_clk_speed_hz,
        .clk_flags = 0,
    };

    /* Silence the i2c_bus library while probing — wrong-address probe is
     * normal and expected here, not an error. */
    esp_log_level_t prev_i2c_master = esp_log_level_get("i2c.master");
    esp_log_level_t prev_i2c_bus = esp_log_level_get("i2c_bus");
    esp_log_level_set("i2c.master", ESP_LOG_NONE);
    esp_log_level_set("i2c_bus", ESP_LOG_NONE);

    bool found = false;
    i2c_bus_handle_t bus = i2c_bus_create(config->i2c_port, &i2c_conf);
    if (bus) {
        i2c_bus_device_handle_t dev = i2c_bus_device_create(bus, config->i2c_addr, config->i2c_clk_speed_hz);
        if (dev) {
            uint8_t chip_id = 0;
            if (i2c_bus_read_bytes(dev, BME68X_REG_CHIP_ID, 1, &chip_id) == ESP_OK && chip_id == BME68X_CHIP_ID) {
                found = true;
            }
            i2c_bus_device_delete(&dev);
        }
        i2c_bus_delete(&bus);
    }

    esp_log_level_set("i2c.master", prev_i2c_master);
    esp_log_level_set("i2c_bus", prev_i2c_bus);

    return found;
}

bool bme680_sensor_is_initialized(void)
{
    return s_ctx.initialized;
}

uint32_t bme680_sensor_get_next_call_delay_ms(void)
{
    if (!s_ctx.initialized) {
        return BSEC_DEFAULT_NEXT_CALL_DELAY_MS;
    }
    return s_ctx.next_call_delay_ms;
}
