#include "sim_data.h"

#include <stdbool.h>

#include "lvgl.h"

#include "sim_log.h"

#define FIRST_MEASUREMENT_MS 3000
#define COUNT(array) (sizeof(array) / sizeof((array)[0]))

typedef struct {
    uint16_t battery_mv;
    uint8_t percent;
} battery_t;

// One value per level the pages tell apart.
static const float IAQ[] = {25, 75, 150, 250, 350, 450};
static const float TEMPERATURE_C[] = {21, 30, -5};
static const float HUMIDITY_PERCENT[] = {45, 85, 100, 20};
static const battery_t BATTERY[] = {{4150, 95}, {3950, 72}, {3800, 47}, {3720, 18}, {3650, 4}};

static size_t s_iaq;
static size_t s_temperature;
static size_t s_humidity;
static size_t s_battery;
static bool s_calibrating;
static bool s_error;
static bool s_usb;

void sim_data_get_snapshot(sensing_snapshot_t* out)
{
    if (lv_tick_get() < FIRST_MEASUREMENT_MS) {
        *out = (sensing_snapshot_t){.status = SENSING_STATUS_NO_DATA};

        return;
    }

    *out = (sensing_snapshot_t){
        .status = s_error ? SENSING_STATUS_ERROR : SENSING_STATUS_OK,
        .temperature_c = TEMPERATURE_C[s_temperature],
        .humidity_percent = HUMIDITY_PERCENT[s_humidity],
        .pressure_hpa = 1013,
        .iaq = IAQ[s_iaq],
        .iaq_accuracy = s_calibrating ? 0 : 3,
    };
}

void sim_data_get_power(power_status_t* out)
{
    if (s_usb) {
        *out = (power_status_t){.source = POWER_SOURCE_EXTERNAL, .battery_mv = 4700};

        return;
    }

    *out = (power_status_t){
        .source = POWER_SOURCE_BATTERY,
        .percent_valid = true,
        .percent = BATTERY[s_battery].percent,
        .battery_mv = BATTERY[s_battery].battery_mv,
    };
}

static size_t next(size_t index, size_t count)
{
    return (index + 1) % count;
}

void sim_data_key(char key)
{
    switch (key) {
        case 'i':
            s_iaq = next(s_iaq, COUNT(IAQ));
            sim_log("data", "IAQ %.0f", IAQ[s_iaq]);
            break;
        case 't':
            s_temperature = next(s_temperature, COUNT(TEMPERATURE_C));
            sim_log("data", "temperature %.0f °C", TEMPERATURE_C[s_temperature]);
            break;
        case 'h':
            s_humidity = next(s_humidity, COUNT(HUMIDITY_PERCENT));
            sim_log("data", "humidity %.0f %%", HUMIDITY_PERCENT[s_humidity]);
            break;
        case 'b':
            s_battery = next(s_battery, COUNT(BATTERY));
            sim_log("data", "battery %u %%, %u mV", BATTERY[s_battery].percent, BATTERY[s_battery].battery_mv);
            break;
        case 'c':
            s_calibrating = !s_calibrating;
            sim_log("data", "IAQ calibration %s", s_calibrating ? "on" : "off");
            break;
        case 'e':
            s_error = !s_error;
            sim_log("data", "sensor error %s", s_error ? "on" : "off");
            break;
        case 'u':
            s_usb = !s_usb;
            sim_log("data", "USB %s", s_usb ? "plugged in" : "unplugged");
            break;
        default:
            break;
    }
}
