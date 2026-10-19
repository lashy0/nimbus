#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sensing_internal.h"

static const char* TAG = "sensing_bsec";

// bme680_iaq_33v_3s_4d/bsec_iaq.config: a little-endian uint32 length, then the blob.
extern const uint8_t s_config_start[] asm("_binary_bsec_iaq_config_start");
extern const uint8_t s_config_end[] asm("_binary_bsec_iaq_config_end");

#define CONFIG_HEADER_BYTES 4

static const uint8_t OUTPUTS[] = {
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_STABILIZATION_STATUS,
    BSEC_OUTPUT_RUN_IN_STATUS,
};

#define OUTPUT_COUNT (sizeof(OUTPUTS) / sizeof(OUTPUTS[0]))

static esp_err_t load_config(void* instance)
{
    const size_t size = s_config_end - s_config_start;
    ESP_RETURN_ON_FALSE(size > CONFIG_HEADER_BYTES, ESP_ERR_INVALID_SIZE, TAG, "config too short");

    const uint32_t declared =
        s_config_start[0] | (s_config_start[1] << 8) | (s_config_start[2] << 16) | ((uint32_t)s_config_start[3] << 24);
    ESP_RETURN_ON_FALSE(
        declared == size - CONFIG_HEADER_BYTES,
        ESP_ERR_INVALID_SIZE,
        TAG,
        "config declares %lu bytes, holds %u",
        declared,
        size - CONFIG_HEADER_BYTES);

    uint8_t* work = malloc(BSEC_MAX_WORKBUFFER_SIZE);
    ESP_RETURN_ON_FALSE(work != NULL, ESP_ERR_NO_MEM, TAG, "work buffer");
    const bsec_library_return_t rc = bsec_set_configuration_m(
        instance, s_config_start + CONFIG_HEADER_BYTES, declared, work, BSEC_MAX_WORKBUFFER_SIZE);
    free(work);
    ESP_RETURN_ON_FALSE(rc == BSEC_OK, ESP_FAIL, TAG, "set configuration: %d", rc);

    return ESP_OK;
}

// Any warning counts as failure: 14 (sample rate mismatch) means the LP outputs
// do not match the loaded configuration.
static esp_err_t subscribe(void* instance)
{
    bsec_sensor_configuration_t requested[OUTPUT_COUNT];
    for (size_t i = 0; i < OUTPUT_COUNT; i++) {
        requested[i] = (bsec_sensor_configuration_t){.sensor_id = OUTPUTS[i], .sample_rate = BSEC_SAMPLE_RATE_LP};
    }

    bsec_sensor_configuration_t required[BSEC_MAX_PHYSICAL_SENSOR];
    uint8_t required_count = BSEC_MAX_PHYSICAL_SENSOR;
    const bsec_library_return_t rc =
        bsec_update_subscription_m(instance, requested, OUTPUT_COUNT, required, &required_count);
    ESP_RETURN_ON_FALSE(rc == BSEC_OK, ESP_FAIL, TAG, "update subscription: %d", rc);

    return ESP_OK;
}

esp_err_t sensing_bsec_open(sensing_bsec_t* bsec)
{
    void* instance = malloc(bsec_get_instance_size_m());
    ESP_RETURN_ON_FALSE(instance != NULL, ESP_ERR_NO_MEM, TAG, "instance");

    esp_err_t ret = ESP_OK; // Set by the ESP_GOTO_ON_* macros.
    const bsec_library_return_t rc = bsec_init_m(instance);
    ESP_GOTO_ON_FALSE(rc == BSEC_OK, ESP_FAIL, fail, TAG, "init: %d", rc);
    ESP_GOTO_ON_ERROR(load_config(instance), fail, TAG, "configuration");
    ESP_GOTO_ON_ERROR(subscribe(instance), fail, TAG, "subscription");

    bsec_version_t version;
    if (bsec_get_version_m(instance, &version) == BSEC_OK) {
        ESP_LOGI(
            TAG,
            "BSEC %u.%u.%u.%u, instance %u bytes",
            version.major,
            version.minor,
            version.major_bugfix,
            version.minor_bugfix,
            bsec_get_instance_size_m());
    }

    bsec->instance = instance;
    bsec->next_call_ns = 0;

    return ESP_OK;

fail:
    free(instance);

    return ret;
}

static void store_outputs(const bsec_output_t* outputs, uint8_t count, sensing_snapshot_t* snapshot)
{
    for (uint8_t i = 0; i < count; i++) {
        const bsec_output_t* output = &outputs[i];
        switch (output->sensor_id) {
            case BSEC_OUTPUT_IAQ:
                snapshot->iaq_accuracy = output->accuracy;
                break;
            case BSEC_OUTPUT_STATIC_IAQ:
                snapshot->static_iaq = output->signal;
                snapshot->iaq_valid = true;
                break;
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
                snapshot->temperature_c = output->signal;
                snapshot->temperature_valid = true;
                break;
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
                snapshot->humidity_percent = output->signal;
                snapshot->humidity_valid = true;
                break;
            case BSEC_OUTPUT_RAW_PRESSURE:
                snapshot->pressure_pa = output->signal;
                snapshot->pressure_valid = true;
                break;
            case BSEC_OUTPUT_STABILIZATION_STATUS:
                snapshot->stabilized = output->signal >= 0.5f;
                break;
            case BSEC_OUTPUT_RUN_IN_STATUS:
                snapshot->run_in = output->signal >= 0.5f;
                break;
            default:
                break;
        }
    }
}

static void add_input(bsec_input_t* inputs, uint8_t* count, uint8_t id, float signal, int64_t time_ns)
{
    inputs[*count] = (bsec_input_t){.sensor_id = id, .signal = signal, .time_stamp = time_ns};
    (*count)++;
}

static uint8_t collect_inputs(
    const bsec_bme_settings_t* settings,
    const struct bme68x_data* data,
    int64_t time_ns,
    bsec_input_t* inputs)
{
    const uint32_t wanted = settings->process_data;
    const bool gas_valid = (data->status & BME68X_GASM_VALID_MSK) != 0;
    uint8_t count = 0;

    if (wanted & BSEC_PROCESS_TEMPERATURE) {
        add_input(inputs, &count, BSEC_INPUT_TEMPERATURE, data->temperature, time_ns);
    }

    if (wanted & BSEC_PROCESS_HUMIDITY) {
        add_input(inputs, &count, BSEC_INPUT_HUMIDITY, data->humidity, time_ns);
    }

    // Pa, as documented for BSEC_INPUT_PRESSURE in bsec_datatypes.h.
    if (wanted & BSEC_PROCESS_PRESSURE) {
        add_input(inputs, &count, BSEC_INPUT_PRESSURE, data->pressure, time_ns);
    }

    if ((wanted & BSEC_PROCESS_GAS) && gas_valid) {
        add_input(inputs, &count, BSEC_INPUT_GASRESISTOR, data->gas_resistance, time_ns);
    }

    // Forced mode has a single heater step, so the profile part is always 0.
    if ((wanted & BSEC_PROCESS_PROFILE_PART) && gas_valid) {
        add_input(inputs, &count, BSEC_INPUT_PROFILE_PART, 0, time_ns);
    }

    return count;
}

esp_err_t sensing_bsec_step(sensing_bsec_t* bsec, sensing_sensor_t* sensor, sensing_snapshot_t* snapshot, bool* updated)
{
    *updated = false;

    const int64_t now_ns = esp_timer_get_time() * 1000;
    bsec_bme_settings_t settings;
    bsec_library_return_t rc = bsec_sensor_control_m(bsec->instance, now_ns, &settings);
    ESP_RETURN_ON_FALSE(rc >= BSEC_OK, ESP_FAIL, TAG, "sensor control: %d", rc);
    if (rc != BSEC_OK) {
        ESP_LOGW(TAG, "sensor control: %d", rc); // 100: called too late, e.g. after a retry.
    }

    bsec->next_call_ns = settings.next_call;
    if (!settings.trigger_measurement || settings.op_mode == BME68X_SLEEP_MODE) {
        return ESP_OK;
    }

    ESP_RETURN_ON_FALSE(
        settings.op_mode == BME68X_FORCED_MODE,
        ESP_ERR_NOT_SUPPORTED,
        TAG,
        "BME68x mode %u is not supported",
        settings.op_mode);

    struct bme68x_data data;
    bool has_data = false;
    ESP_RETURN_ON_ERROR(sensing_sensor_measure(sensor, &settings, &data, &has_data), TAG, "measurement");
    if (!has_data || !(data.status & BME68X_NEW_DATA_MSK)) {
        ESP_LOGW(TAG, "measurement returned no new data");

        return ESP_OK;
    }

    bsec_input_t inputs[BSEC_MAX_PHYSICAL_SENSOR];
    const uint8_t input_count = collect_inputs(&settings, &data, now_ns, inputs);
    if (input_count == 0) {
        return ESP_OK;
    }

    bsec_output_t outputs[BSEC_NUMBER_OUTPUTS];
    uint8_t output_count = BSEC_NUMBER_OUTPUTS;
    rc = bsec_do_steps_m(bsec->instance, inputs, input_count, outputs, &output_count);
    ESP_RETURN_ON_FALSE(rc >= BSEC_OK, ESP_FAIL, TAG, "do steps: %d", rc);
    if (rc != BSEC_OK) {
        ESP_LOGW(TAG, "do steps: %d", rc);
    }

    store_outputs(outputs, output_count, snapshot);
    *updated = output_count > 0;

    return ESP_OK;
}
