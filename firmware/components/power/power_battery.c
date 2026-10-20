#include "power_battery.h"

#include <stddef.h>
#include <string.h>

#define FILTER_SHIFT 3     // Each reading moves the filter by 1/8: about 16 s at a 2 s poll.
#define PERCENT_RISE_MIN 3 // A smaller rise on battery is load recovery, not charge.

typedef struct {
    uint16_t mv;
    uint8_t percent;
} curve_point_t;

// Typical single-cell Li-ion discharge under light load, not a profile of this cell.
static const curve_point_t CURVE[] = {
    {4200, 100}, {4150, 95}, {4110, 90}, {4080, 85}, {4020, 80}, {3980, 75}, {3950, 70},
    {3910, 65},  {3870, 60}, {3850, 55}, {3840, 50}, {3820, 45}, {3800, 40}, {3790, 35},
    {3770, 30},  {3750, 25}, {3730, 20}, {3710, 15}, {3690, 10}, {3610, 5},  {3300, 0},
};

#define CURVE_COUNT (sizeof(CURVE) / sizeof(CURVE[0]))

uint8_t power_percent_from_mv(uint32_t battery_mv)
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

static bool plausible(uint32_t battery_mv)
{
    return battery_mv >= POWER_PLAUSIBLE_MIN_MV && battery_mv <= POWER_PLAUSIBLE_MAX_MV;
}

void power_estimator_init(power_estimator_t* estimator)
{
    memset(estimator, 0, sizeof(*estimator));
    estimator->status.source = POWER_SOURCE_UNKNOWN;
}

static void update_percent(power_estimator_t* estimator, uint32_t battery_mv)
{
    power_status_t* status = &estimator->status;
    if (estimator->filtered_mv_x16 == 0) {
        estimator->filtered_mv_x16 = (int32_t)battery_mv << 4;
    } else {
        estimator->filtered_mv_x16 += (((int32_t)battery_mv << 4) - estimator->filtered_mv_x16) >> FILTER_SHIFT;
    }

    const uint8_t percent = power_percent_from_mv((uint32_t)(estimator->filtered_mv_x16 >> 4));
    if (!status->percent_valid || percent < status->percent || percent >= status->percent + PERCENT_RISE_MIN) {
        status->percent = percent;
    }

    status->percent_valid = true;
}

void power_estimator_update(power_estimator_t* estimator, bool ok, uint32_t battery_mv)
{
    power_status_t* status = &estimator->status;

    // A failed or implausible read keeps the low-reading count: it proves nothing either way.
    if (!ok || !plausible(battery_mv)) {
        status->source = POWER_SOURCE_UNKNOWN;
        status->percent_valid = false;
        estimator->filtered_mv_x16 = 0;

        return;
    }

    status->battery_mv = (uint16_t)battery_mv;
    if (battery_mv >= POWER_EXTERNAL_MIN_MV) {
        status->source = POWER_SOURCE_EXTERNAL;
        status->percent_valid = false;
        estimator->filtered_mv_x16 = 0;
        estimator->low_readings = 0;
        status->critical = false;

        return;
    }

    status->source = POWER_SOURCE_BATTERY;
    update_percent(estimator, battery_mv);

    if (battery_mv < POWER_CRITICAL_MV) {
        if (estimator->low_readings < POWER_CRITICAL_READINGS) {
            estimator->low_readings++;
        }
    } else {
        estimator->low_readings = 0;
    }

    status->critical = estimator->low_readings >= POWER_CRITICAL_READINGS;
}

bool power_startup_allowed(bool ok, uint32_t battery_mv, bool after_brownout)
{
    if (!ok || !plausible(battery_mv)) {
        return !after_brownout;
    }

    return battery_mv >= (after_brownout ? POWER_STARTUP_AFTER_BROWNOUT_MV : POWER_STARTUP_MIN_MV);
}
