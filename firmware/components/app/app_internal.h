#pragma once

#include "app_state.h"

typedef enum {
    APP_SENSOR_WAITING, // No output since boot yet; the start screen stays.
    APP_SENSOR_FAULT,   // One error page replaces the measurement pages.
    APP_SENSOR_WORKING,
} app_sensor_status_t;

bool app_is_main_screen(ui_screen_t screen);
app_sensor_status_t app_sensor_status(const app_state_t* state);

uint32_t app_save_settings(app_state_t* state);
uint32_t app_begin_shutdown(app_state_t* state, app_shutdown_reason_t reason);

uint32_t app_display_request(app_state_t* state, ui_display_state_t display);
uint32_t app_display_off(app_state_t* state);
uint32_t app_display_close_usb_notice(app_state_t* state);

// Call before replacing state->power so a plug-in is distinguished from USB at boot.
uint32_t app_display_power(app_state_t* state, const power_status_t* power, int64_t now_ms);

// Updates activity and consumes DIM/OFF wake gestures and gestures over the USB notice.
uint32_t app_display_button(app_state_t* state, const app_button_event_t* event, bool* consumed);

// Idle deadline for DIM or OFF; disabled while a button is held or outside ACTIVE.
int64_t app_display_idle_deadline(const app_state_t* state, ui_display_state_t display);
