#include "preview_readings.h"

#include "app.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"

#define SAMPLE_PERIOD_US (4 * 1000 * 1000)

static const char* TAG = "preview";

#define READINGS(iaq_value, accuracy, temperature, humidity)                                                           \
    .iaq_valid = true, .iaq_accuracy = (accuracy), .iaq = (iaq_value), .temperature_valid = true,                      \
    .temperature_c = (temperature), .humidity_valid = true, .humidity_percent = (humidity)

// One sample per IAQ level and temperature/humidity band, plus calibration,
// charging and an unknown battery.
static const app_readings_t SAMPLES[] = {
    {READINGS(25.0f, 0, 15.2f, 45.0f), .battery_valid = true, .battery_percent = 100},
    {READINGS(31.0f, 3, -40.0f, 14.0f), .battery_valid = true, .battery_percent = 100, .charging = true},
    {READINGS(69.0f, 3, 15.0f, 45.0f), .battery_valid = true, .battery_percent = 85},
    {READINGS(151.0f, 3, 35.0f, 100.0f), .battery_valid = true, .battery_percent = 60, .charging = true},
    {READINGS(210.0f, 3, 22.4f, 55.0f), .battery_valid = true, .battery_percent = 45},
    {READINGS(322.0f, 3, -5.0f, 72.0f), .battery_valid = true, .battery_percent = 20},
    {READINGS(499.0f, 3, 30.0f, 25.0f), .battery_valid = false},
};

#define SAMPLE_COUNT (sizeof(SAMPLES) / sizeof(SAMPLES[0]))

static size_t s_next;

static void post_next_sample(void* arg)
{
    const app_readings_t* sample = &SAMPLES[s_next];
    ESP_LOGI(TAG,
        "sample %u/%u: IAQ %.0f (accuracy %u), %.1f C, %.0f %%RH",
        s_next + 1,
        SAMPLE_COUNT,
        sample->iaq,
        sample->iaq_accuracy,
        sample->temperature_c,
        sample->humidity_percent);

    app_post_readings(sample);
    s_next = (s_next + 1) % SAMPLE_COUNT;
}

esp_err_t preview_readings_start(void)
{
    ESP_LOGW(TAG, "showing synthetic readings, not measurements");

    const esp_timer_create_args_t args = {
        .callback = post_next_sample,
        .name = "preview",
    };
    esp_timer_handle_t timer;
    ESP_RETURN_ON_ERROR(esp_timer_create(&args, &timer), TAG, "timer");

    post_next_sample(NULL);

    return esp_timer_start_periodic(timer, SAMPLE_PERIOD_US);
}
