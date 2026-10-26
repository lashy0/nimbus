#include "app_internal.h"

#include <math.h>
#include <string.h>

#define IAQ_MAX 500
#define TEMPERATURE_MIN_C -40 // BME680 operating range.
#define TEMPERATURE_MAX_C 85
#define HUMIDITY_MAX 100

static long round_clamped(float value, long min, long max)
{
    const long rounded = lroundf(value);
    if (rounded < min) {
        return min;
    }

    return rounded > max ? max : rounded;
}

static ui_screen_t shown_screen(const app_state_t* state, app_sensor_status_t status)
{
    if (!app_is_main_screen(state->screen)) {
        return state->screen;
    }

    switch (status) {
        case APP_SENSOR_WAITING:
            return UI_SCREEN_START;
        case APP_SENSOR_FAULT:
            return UI_SCREEN_SENSOR_ERROR;
        case APP_SENSOR_WORKING:
            break;
    }

    return state->screen;
}

void app_state_model(const app_state_t* state, ui_model_t* model)
{
    const sensing_snapshot_t* measurement = &state->measurement;
    const app_sensor_status_t status = app_sensor_status(state);
    memset(model, 0, sizeof(*model));

    model->screen = shown_screen(state, status);
    model->usb_notice = state->notice_deadline_ms != APP_NO_DEADLINE;
    model->brightness_percent = state->brightness_percent;
    model->shutdown_yes_selected = state->shutdown_yes_selected;

    if (state->power.source == POWER_SOURCE_EXTERNAL) {
        model->battery.state = UI_BATTERY_EXTERNAL_POWER;
    } else if (state->power.percent_valid) {
        model->battery.state = UI_BATTERY_LEVEL;
        model->battery.percent = state->power.percent;
    }

    if (status != APP_SENSOR_WORKING) {
        return;
    }

    ui_measurements_t* values = &model->measurements;
    values->iaq.calibrating = measurement->phase == SENSING_PHASE_WARMUP;
    if (!values->iaq.calibrating) {
        values->iaq.value = (uint16_t)round_clamped(measurement->iaq, 0, IAQ_MAX);
    }

    values->temperature_c = (int16_t)round_clamped(measurement->temperature_c, TEMPERATURE_MIN_C, TEMPERATURE_MAX_C);
    values->humidity_percent = (uint8_t)round_clamped(measurement->humidity_percent, 0, HUMIDITY_MAX);
}
