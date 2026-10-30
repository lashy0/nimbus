#pragma once

#include <stdint.h>

/**
 * @file
 * Screen hardware, provided by the platform. Called with exclusive LVGL access.
 */

/** Sets the backlight percent; 0 darkens the screen, and the next nonzero value lights it again. */
void ui_port_set_backlight(uint8_t percent);

/** Keeps the brightness chosen on the brightness page across restarts. */
void ui_port_save_brightness(uint8_t percent);
