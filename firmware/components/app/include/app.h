#pragma once

#include <stdint.h>

#include "app_events.h"
#include "esp_err.h"

typedef struct {
    uint8_t brightness_percent; // Current backlight level set by ui_init().
} app_config_t;

// Starts app_task, the only writer of the application state. Call once after
// ui_init(); without a display the state still runs and the UI calls fail quietly.
esp_err_t app_start(const app_config_t* config);

// Never blocks; safe from the esp_timer task. A full queue is reported by app_task
// and cancels an open shutdown question rather than acting on partial input.
void app_post_button(const app_button_event_t* event);

// Replaces the latest readings; several posts before app_task runs coalesce.
void app_post_readings(const app_readings_t* readings);
