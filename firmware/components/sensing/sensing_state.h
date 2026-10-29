#pragma once

#include "esp_err.h"

/**
 * @file
 * Keeps the BSEC state, calibration included, in NVS across restarts.
 * Needs initialized NVS. Not thread-safe, like sensing_bsec.h.
 */

/**
 * Restores the saved state into BSEC.
 *
 * Call between sensing_bsec_init() and sensing_bsec_subscribe().
 *
 * @retval ESP_OK The state is restored.
 * @retval ESP_ERR_NOT_FOUND Nothing has been saved yet.
 * @retval ESP_FAIL BSEC rejected the saved state.
 * @retval other NVS errors.
 */
esp_err_t sensing_state_load(void);

/**
 * Saves the current BSEC state, replacing the previous one.
 *
 * @retval ESP_OK The state is committed to flash.
 * @retval ESP_FAIL BSEC failed to return the state.
 * @retval other NVS errors.
 */
esp_err_t sensing_state_save(void);
