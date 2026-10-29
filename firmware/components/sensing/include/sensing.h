#pragma once

#include "driver/i2c_types.h"
#include "esp_err.h"

#include "sensing_types.h"

/**
 * @file
 * Air measurements: owns the BME680 and BSEC, measures on the BSEC schedule in
 * its own task and keeps the latest result as a snapshot.
 */

/**
 * Opens the sensor, starts BSEC and the sensing task.
 *
 * Call once, from a task. The bus is borrowed and must outlive sensing.
 *
 * @param bus Initialized I2C master bus.
 * @retval ESP_OK The task is running; the first snapshot follows the first measurement.
 * @retval ESP_ERR_INVALID_STATE Sensing has already started.
 * @retval ESP_ERR_NOT_FOUND No BME680 answered on the bus.
 * @retval ESP_ERR_NO_MEM The sensor handle or the task could not be allocated.
 * @retval ESP_FAIL The sensor or BSEC failed to initialize.
 */
esp_err_t sensing_start(i2c_master_bus_handle_t bus);

/**
 * Copies the latest snapshot without triggering a measurement.
 *
 * Safe from any task, but not from an ISR. Before the first measurement the
 * status is SENSING_STATUS_NO_DATA.
 *
 * @param out Non-NULL destination owned by the caller.
 */
void sensing_get_snapshot(sensing_snapshot_t* out);
