#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#include "power_types.h"

/**
 * Reads the battery node in millivolts from the calling task.
 * Must return within a few milliseconds. The output is used only on ESP_OK.
 */
typedef esp_err_t (*power_read_battery_fn)(void* ctx, uint32_t* battery_mv);

/** Enters the final deep sleep from the calling task and never returns. */
typedef void (*power_sleep_fn)(void* ctx);

typedef enum {
    POWER_SLEEP_USER,     /**< Power-off confirmed on screen. */
    POWER_SLEEP_CRITICAL, /**< Confirmed battery discharge. */
    POWER_SLEEP_STARTUP,  /**< Not enough voltage to start the display and sensor. */
    POWER_SLEEP_FAULT,    /**< The device cannot work: a failed display, startup or repeated crashes. */
} power_sleep_reason_t;

typedef struct {
    power_read_battery_fn read_battery;
    void* read_ctx; /**< Borrowed context for read_battery; must remain valid until final sleep. */
    power_sleep_fn enter_sleep;
    void* sleep_ctx; /**< Borrowed context for enter_sleep; must remain valid until final sleep. */
} power_config_t;

/**
 * Copies the callbacks and initializes the battery estimator.
 *
 * Call before other power APIs. All calls run synchronously in the calling task;
 * the component creates no task and provides no locking. After a wake it logs
 * why and at which voltage the device went to sleep.
 *
 * @param config Non-NULL configuration with both callbacks set.
 * @retval ESP_OK Initialization completed.
 * @retval ESP_ERR_INVALID_ARG The configuration or a callback is NULL.
 */
esp_err_t power_init(const power_config_t* config);

/**
 * Reads the battery once and copies the updated estimate to non-NULL out.
 * Call from one task after power_init(); may block in the read callback.
 * A failed read produces UNKNOWN with an invalid percentage; battery_mv retains
 * the last plausible reading and is not a new measurement in that case.
 */
void power_poll(power_status_t* out);

/**
 * Reads the battery and returns whether display and sensor startup is allowed.
 * Call after power_init(), before starting those consumers. May block in the
 * read callback. Unknown voltage is allowed except after a brownout, which needs
 * a margin above the normal start threshold. Also updates the battery estimator.
 */
bool power_check_startup(bool after_brownout);

/**
 * Records the reason in RTC memory for the next boot, then sleeps without returning.
 * Call from a task after power_init() and after consumers have been shut down.
 */
__attribute__((noreturn)) void power_enter_sleep(power_sleep_reason_t reason);
