#pragma once

#include "ui.h"

// Acquires the light-sleep lock before the renderer starts. Panel and backlight
// handles are borrowed from main and remain valid for the rest of the boot.
esp_err_t ui_display_init(const ui_config_t* config);

// Darkens the backlight on every failed start. Releases the PM lock and hands
// the pin over only after rendering is confirmed stopped; panel handles stay borrowed.
esp_err_t ui_display_abort_init(bool renderer_stopped);

// Draws the start screen before exposing panel RAM or lighting the backlight.
esp_err_t ui_display_start(uint8_t brightness_percent);

// Continues an interrupted hardware transition from its last completed step.
esp_err_t ui_display_set_state(ui_display_state_t state, uint8_t brightness_percent);
