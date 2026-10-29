#include "sensing_bsec.h"

#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "bme68x_defs.h"
#include "bsec_interface.h"

// How much the board heats the sensor, °C. Not measured yet.
#define TEMPERATURE_OFFSET_C 0.0f

static const char* TAG = "sensing_bsec";

static const uint8_t CONFIG[] = {
#include "bme680/bme680_iaq_33v_3s_4d/bsec_iaq.txt"
};

static const uint8_t OUTPUTS[] = {
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
    BSEC_OUTPUT_RAW_PRESSURE,
};

#define OUTPUT_COUNT (sizeof(OUTPUTS) / sizeof(OUTPUTS[0]))

static uint8_t s_work[BSEC_MAX_WORKBUFFER_SIZE];

esp_err_t sensing_bsec_init(void)
{
    bsec_library_return_t rc = bsec_init();
    ESP_RETURN_ON_FALSE(rc == BSEC_OK, ESP_FAIL, TAG, "init: %d", rc);

    rc = bsec_set_configuration(CONFIG, sizeof(CONFIG), s_work, sizeof(s_work));
    ESP_RETURN_ON_FALSE(rc == BSEC_OK, ESP_FAIL, TAG, "set configuration: %d", rc);

    bsec_version_t version;
    if (bsec_get_version(&version) == BSEC_OK) {
        ESP_LOGI(TAG, "BSEC %u.%u.%u.%u", version.major, version.minor, version.major_bugfix, version.minor_bugfix);
    }

    return ESP_OK;
}

esp_err_t sensing_bsec_set_state(const uint8_t* state, uint32_t size)
{
    const bsec_library_return_t rc = bsec_set_state(state, size, s_work, sizeof(s_work));
    ESP_RETURN_ON_FALSE(rc == BSEC_OK, ESP_FAIL, TAG, "set state: %d", rc);

    return ESP_OK;
}

esp_err_t sensing_bsec_get_state(uint8_t state[BSEC_MAX_STATE_BLOB_SIZE], uint32_t* size)
{
    const bsec_library_return_t rc = bsec_get_state(0, state, BSEC_MAX_STATE_BLOB_SIZE, s_work, sizeof(s_work), size);
    ESP_RETURN_ON_FALSE(rc == BSEC_OK, ESP_FAIL, TAG, "get state: %d", rc);

    return ESP_OK;
}

// Warnings count as errors: they mean the outputs do not match the configuration.
esp_err_t sensing_bsec_subscribe(void)
{
    bsec_sensor_configuration_t requested[OUTPUT_COUNT];
    for (size_t i = 0; i < OUTPUT_COUNT; i++) {
        requested[i] = (bsec_sensor_configuration_t){.sensor_id = OUTPUTS[i], .sample_rate = BSEC_SAMPLE_RATE_LP};
    }

    bsec_sensor_configuration_t required[BSEC_MAX_PHYSICAL_SENSOR];
    uint8_t required_count = BSEC_MAX_PHYSICAL_SENSOR;
    const bsec_library_return_t rc = bsec_update_subscription(requested, OUTPUT_COUNT, required, &required_count);
    ESP_RETURN_ON_FALSE(rc == BSEC_OK, ESP_FAIL, TAG, "update subscription: %d", rc);

    return ESP_OK;
}

static void add_input(bsec_input_t* inputs, uint8_t* count, uint8_t id, float signal, int64_t time_ns)
{
    inputs[*count] = (bsec_input_t){
        .time_stamp = time_ns,
        .signal = signal,
        .signal_dimensions = 1,
        .sensor_id = id,
    };
    (*count)++;
}

static bool is_requested(uint32_t process_data, uint8_t id)
{
    return (process_data & (1UL << (id - 1))) != 0;
}

static uint8_t collect_inputs(
    const bsec_bme_settings_t* settings,
    const bme680_sample_t* sample,
    int64_t time_ns,
    bsec_input_t* inputs)
{
    const uint32_t wanted = settings->process_data;
    uint8_t count = 0;

    if (is_requested(wanted, BSEC_INPUT_HEATSOURCE)) {
        add_input(inputs, &count, BSEC_INPUT_HEATSOURCE, TEMPERATURE_OFFSET_C, time_ns);
    }

    if (is_requested(wanted, BSEC_INPUT_TEMPERATURE)) {
        add_input(inputs, &count, BSEC_INPUT_TEMPERATURE, sample->temperature_c, time_ns);
    }

    if (is_requested(wanted, BSEC_INPUT_HUMIDITY)) {
        add_input(inputs, &count, BSEC_INPUT_HUMIDITY, sample->humidity_percent, time_ns);
    }

    if (is_requested(wanted, BSEC_INPUT_PRESSURE)) {
        add_input(inputs, &count, BSEC_INPUT_PRESSURE, sample->pressure_pa / 100.0f, time_ns);
    }

    if (is_requested(wanted, BSEC_INPUT_GASRESISTOR)) {
        add_input(inputs, &count, BSEC_INPUT_GASRESISTOR, sample->gas_ohm, time_ns);
    }

    // Forced mode has a single heater step.
    if (is_requested(wanted, BSEC_INPUT_PROFILE_PART)) {
        add_input(inputs, &count, BSEC_INPUT_PROFILE_PART, 0, time_ns);
    }

    return count;
}

static void store_outputs(const bsec_output_t* outputs, uint8_t count, sensing_bsec_outputs_t* out)
{
    out->count = count;

    for (uint8_t i = 0; i < count; i++) {
        const bsec_output_t* output = &outputs[i];

        switch (output->sensor_id) {
            case BSEC_OUTPUT_IAQ:
                out->iaq = output->signal;
                out->iaq_accuracy = output->accuracy;
                break;
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
                out->temperature_c = output->signal;
                break;
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
                out->humidity_percent = output->signal;
                break;
            case BSEC_OUTPUT_RAW_PRESSURE:
                out->pressure_hpa = output->signal;
                break;
            default:
                break;
        }
    }
}

esp_err_t sensing_bsec_step(bme680_handle_t sensor, sensing_bsec_outputs_t* out, int64_t* next_call_ns)
{
    *out = (sensing_bsec_outputs_t){0};

    const int64_t now_ns = esp_timer_get_time() * 1000;
    bsec_bme_settings_t settings;
    bsec_library_return_t rc = bsec_sensor_control(now_ns, &settings);
    ESP_RETURN_ON_FALSE(rc >= BSEC_OK, ESP_FAIL, TAG, "sensor control: %d", rc);
    if (rc > BSEC_OK) {
        ESP_LOGW(TAG, "sensor control: warning %d", rc);
    }

    *next_call_ns = settings.next_call;
    if (!settings.trigger_measurement) {
        return ESP_OK;
    }

    ESP_RETURN_ON_FALSE(
        settings.op_mode == BME68X_FORCED_MODE, ESP_ERR_NOT_SUPPORTED, TAG, "op mode %u", settings.op_mode);

    const bme680_config_t config = {
        .temperature_os = settings.temperature_oversampling,
        .pressure_os = settings.pressure_oversampling,
        .humidity_os = settings.humidity_oversampling,
        .gas_enabled = settings.run_gas,
        .heater_temp_c = settings.heater_temperature,
        .heater_ms = settings.heater_duration,
    };
    ESP_RETURN_ON_ERROR(bme680_configure(sensor, &config), TAG, "configure");

    bme680_sample_t sample;
    ESP_RETURN_ON_ERROR(bme680_measure(sensor, &sample), TAG, "measure");

    if (!sample.gas_valid) {
        ESP_LOGW(TAG, "no valid gas reading, measurement dropped");

        return ESP_OK;
    }

    bsec_input_t inputs[BSEC_MAX_PHYSICAL_SENSOR];
    const uint8_t input_count = collect_inputs(&settings, &sample, now_ns, inputs);
    if (input_count == 0) {
        return ESP_OK;
    }

    bsec_output_t outputs[BSEC_NUMBER_OUTPUTS];
    uint8_t output_count = BSEC_NUMBER_OUTPUTS;
    rc = bsec_do_steps(inputs, input_count, outputs, &output_count);
    ESP_RETURN_ON_FALSE(rc >= BSEC_OK, ESP_FAIL, TAG, "do steps: %d", rc);
    if (rc > BSEC_OK) {
        ESP_LOGW(TAG, "do steps: warning %d", rc);
    }

    store_outputs(outputs, output_count, out);

    return ESP_OK;
}
