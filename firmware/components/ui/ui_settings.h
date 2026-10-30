#pragma once

#include <stdint.h>

#include "esp_err.h"

/**
 * @file
 * UI settings kept in NVS across restarts. Needs initialized NVS.
 */

/**
 * Reads the saved backlight percent.
 *
 * @param out Non-NULL destination; set only on ESP_OK.
 * @retval ESP_OK The value is read.
 * @retval ESP_ERR_NOT_FOUND Nothing has been saved yet.
 * @retval other NVS errors.
 */
esp_err_t ui_settings_load_brightness(uint8_t* out);

/**
 * Saves the backlight percent, replacing the previous one.
 *
 * @retval ESP_OK The value is committed to flash.
 * @retval other NVS errors.
 */
esp_err_t ui_settings_save_brightness(uint8_t percent);
