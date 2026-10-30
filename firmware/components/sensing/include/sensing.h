#pragma once

#include "driver/i2c_types.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#include "sensing_types.h"

/**
 * @file
 * Air measurements. Owns the BME680 and BSEC, measures in its own task and
 * keeps the latest result as a snapshot. Calibration is kept in NVS across restarts.
 */

/**
 * Opens the sensor, restores the saved calibration and starts the sensing task.
 *
 * Call once, from a task. The bus is borrowed and must outlive sensing.
 * Initialize NVS first; without it sensing still runs, but calibrates from
 * scratch on every start.
 *
 * @param bus Initialized I2C master bus.
 * @param core Core for the sensing task, or tskNO_AFFINITY.
 * @retval ESP_OK The task is running; the first snapshot follows the first measurement.
 * @retval ESP_ERR_INVALID_ARG The chip has no such core.
 * @retval ESP_ERR_INVALID_STATE Sensing has already started.
 * @retval ESP_ERR_NOT_FOUND No BME680 answered on the bus.
 * @retval ESP_ERR_NO_MEM The sensor handle or the task could not be allocated.
 * @retval ESP_FAIL The sensor or BSEC failed to initialize.
 */
esp_err_t sensing_start(i2c_master_bus_handle_t bus, BaseType_t core);

/**
 * Copies the latest snapshot without triggering a measurement.
 *
 * Safe from any task, but not from an ISR. Before the first measurement the
 * status is SENSING_STATUS_NO_DATA.
 *
 * @param out Non-NULL destination owned by the caller.
 */
void sensing_get_snapshot(sensing_snapshot_t* out);

/**
 * Stops the sensing task after its current step and saves the calibration.
 * The snapshot keeps its last value. Sensing cannot start again until a reset.
 *
 * Call from a task. Waits up to 2 s.
 *
 * @retval ESP_OK The task has stopped; a failed save is only logged.
 * @retval ESP_ERR_INVALID_STATE Sensing has not started or is already stopped.
 * @retval ESP_ERR_TIMEOUT The task did not stop in time.
 */
esp_err_t sensing_stop(void);
