#include "power_estimate.h"

#include <stddef.h>

// Not checked against a voltmeter.
#define EXTERNAL_MIN_MV 4400 // The pin reads about 4.7 V on USB.
#define PLAUSIBLE_MIN_MV 2500
#define PLAUSIBLE_MAX_MV 6000
#define CRITICAL_MV 3300
#define CRITICAL_READINGS 12 // About 6 s.
#define STARTUP_MIN_MV 3400
#define STARTUP_AFTER_BROWNOUT_MV 3500 // Otherwise the brownout repeats.

#define FILTER_SHIFT 5     // About 16 s.
#define PERCENT_RISE_MIN 3 // A smaller rise is recovery after a load.

typedef struct {
    uint16_t mv;
    uint8_t percent;
} curve_point_t;

// Typical Li-ion cell.
static const curve_point_t CURVE[] = {
    {4177, 100}, {4129, 95}, {4086, 90}, {4045, 85}, {4008, 80}, {3975, 75}, {3945, 70},
    {3918, 65},  {3884, 60}, {3841, 55}, {3821, 50}, {3806, 45}, {3793, 40}, {3784, 35},
    {3775, 30},  {3762, 25}, {3741, 20}, {3710, 15}, {3687, 10}, {3675, 5},  {3306, 0},
};

#define CURVE_COUNT (sizeof(CURVE) / sizeof(CURVE[0]))

static uint8_t percent_of(uint32_t battery_mv)
{
    if (battery_mv >= CURVE[0].mv) {
        return CURVE[0].percent;
    }

    for (size_t i = 1; i < CURVE_COUNT; i++) {
        const curve_point_t* upper = &CURVE[i - 1];
        const curve_point_t* lower = &CURVE[i];
        if (battery_mv >= lower->mv) {
            const uint32_t span_mv = upper->mv - lower->mv;
            const uint32_t span_percent = upper->percent - lower->percent;

            return lower->percent + (uint8_t)(((battery_mv - lower->mv) * span_percent + span_mv / 2) / span_mv);
        }
    }

    return 0;
}

void power_estimate_init(power_estimate_t* estimate)
{
    *estimate = (power_estimate_t){.status.source = POWER_SOURCE_UNKNOWN};
}

static void restart_filter(power_estimate_t* estimate)
{
    estimate->status.percent_valid = false;
    estimate->filtered_mv_x256 = 0;
    estimate->seed_count = 0;
}

// Returns the upper median of the readings so far.
static uint32_t add_seed(power_estimate_t* estimate, uint32_t battery_mv)
{
    uint8_t i = estimate->seed_count++;
    for (; i > 0 && estimate->seed_mv[i - 1] > battery_mv; i--) {
        estimate->seed_mv[i] = estimate->seed_mv[i - 1];
    }
    estimate->seed_mv[i] = (uint16_t)battery_mv;

    return estimate->seed_mv[estimate->seed_count / 2];
}

static void update_percent(power_estimate_t* estimate, uint32_t battery_mv)
{
    power_status_t* status = &estimate->status;

    // A single reading can catch the sag right after USB is unplugged;
    // the filter would then climb from it for a minute.
    if (estimate->filtered_mv_x256 == 0) {
        const uint32_t median_mv = add_seed(estimate, battery_mv);
        if (estimate->seed_count == POWER_ESTIMATE_SEED_READINGS) {
            estimate->filtered_mv_x256 = (int32_t)median_mv << 8;
        }
        status->percent = percent_of(median_mv);
        status->percent_valid = true;

        return;
    }

    estimate->filtered_mv_x256 += (((int32_t)battery_mv << 8) - estimate->filtered_mv_x256) >> FILTER_SHIFT;

    const uint8_t percent = percent_of((uint32_t)(estimate->filtered_mv_x256 >> 8));
    if (percent < status->percent || percent >= status->percent + PERCENT_RISE_MIN) {
        status->percent = percent;
    }
}

void power_estimate_update(power_estimate_t* estimate, bool ok, uint32_t battery_mv)
{
    power_status_t* status = &estimate->status;

    if (!ok || battery_mv < PLAUSIBLE_MIN_MV || battery_mv > PLAUSIBLE_MAX_MV) {
        status->source = POWER_SOURCE_UNKNOWN;
        restart_filter(estimate);

        return;
    }

    status->battery_mv = (uint16_t)battery_mv;
    if (battery_mv >= EXTERNAL_MIN_MV) {
        status->source = POWER_SOURCE_EXTERNAL;
        status->critical = false;
        estimate->low_readings = 0;
        restart_filter(estimate);

        return;
    }

    status->source = POWER_SOURCE_BATTERY;
    update_percent(estimate, battery_mv);

    if (battery_mv >= CRITICAL_MV) {
        estimate->low_readings = 0;
    } else if (estimate->low_readings < CRITICAL_READINGS) {
        estimate->low_readings++;
    }

    status->critical = estimate->low_readings >= CRITICAL_READINGS;
}

bool power_estimate_startup_allowed(const power_status_t* status, bool after_brownout)
{
    switch (status->source) {
        case POWER_SOURCE_UNKNOWN:
            return !after_brownout;
        case POWER_SOURCE_BATTERY:
            return status->battery_mv >= (after_brownout ? STARTUP_AFTER_BROWNOUT_MV : STARTUP_MIN_MV);
        case POWER_SOURCE_EXTERNAL:
            break;
    }

    return true;
}
