#pragma once

#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#include "sensing_types.h"

/** Called from the sensing task after publishing a snapshot; must not block. */
typedef void (*sensing_notify_fn)(void* ctx);

typedef struct {
    i2c_master_bus_handle_t bus; /**< Borrowed for the task lifetime; sensing owns its added device. */
    sensing_notify_fn notify;    /**< Optional publication callback; NULL disables notifications. */
    void* ctx;                   /**< Borrowed callback context; must outlive the sensing task. */
} sensing_config_t;

/**
 * Starts the sensing task, the only owner of the BME680 and BSEC.
 *
 * Call from a task, once per boot. The configuration is copied; its bus and
 * callback context remain borrowed. Sensor setup is asynchronous: a missing or
 * failing sensor is retried with backoff until shutdown is requested.
 * A failed start releases synchronization objects and leaves sensing unavailable.
 *
 * @param config Non-NULL configuration with an initialized I2C bus.
 * @retval ESP_OK The task was created; sensor readiness is reported by phase.
 * @retval ESP_ERR_INVALID_ARG The configuration or bus is NULL.
 * @retval ESP_ERR_INVALID_STATE Sensing has already started, even if stopped.
 * @retval ESP_ERR_NO_MEM Synchronization objects or the task could not be allocated.
 */
esp_err_t sensing_start(const sensing_config_t* config);

/**
 * Requests a final BSEC state save and parks the sensing task for this boot.
 *
 * Call once from another task before power-off. A successful return confirms
 * that the stop path finished, not that the state was saved successfully.
 * A timeout does not cancel the request; the task may still be writing to storage.
 *
 * @param timeout_ms Maximum semaphore wait in milliseconds, converted to RTOS ticks.
 * @retval ESP_OK The task confirmed its stop.
 * @retval ESP_ERR_TIMEOUT No confirmation arrived within the wait.
 * @retval ESP_ERR_INVALID_STATE Sensing was not started or a stop was already requested.
 */
esp_err_t sensing_stop(uint32_t timeout_ms);

/**
 * Copies the latest cached snapshot without triggering a measurement.
 *
 * Before a successful start, returns INITIALIZING. Expired data is returned as
 * ERROR; measurement values are usable only in working phases. Safe from any
 * task after startup; waits for the snapshot mutex and cannot be called from ISR.
 *
 * @param out Non-NULL destination owned by the caller.
 */
void sensing_get_snapshot(sensing_snapshot_t* out);
