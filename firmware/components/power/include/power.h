#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "power_types.h"

// Reads the battery node in millivolts; must return within a few milliseconds.
typedef esp_err_t (*power_read_battery_fn)(void* ctx, uint32_t* battery_mv);

// Enters the final deep sleep and never returns.
typedef void (*power_sleep_fn)(void* ctx);

typedef enum {
    POWER_SLEEP_USER,     // Power-off confirmed on screen.
    POWER_SLEEP_CRITICAL, // Confirmed battery discharge.
    POWER_SLEEP_STARTUP,  // Not enough voltage to start the display and sensor.
} power_sleep_reason_t;

typedef struct {
    power_read_battery_fn read_battery;
    void* read_ctx;
    power_sleep_fn enter_sleep;
    void* sleep_ctx;
} power_config_t;

// No task of its own: every call runs synchronously in the caller. After a wake
// it logs why and at which voltage the device went to sleep.
esp_err_t power_init(const power_config_t* config);

// Reads the battery once and returns the updated estimate. Call from one task.
void power_poll(power_status_t* out);

// One reading before the display and sensor start. An unknown voltage is allowed
// except after a brownout, which needs a margin above the start threshold.
bool power_check_startup(bool after_brownout);

// Records the reason in RTC memory for the next boot, then sleeps.
__attribute__((noreturn)) void power_enter_sleep(power_sleep_reason_t reason);
