#pragma once

#include <stdint.h>

#include "esp_err.h"

#include "app_events.h"

typedef struct {
    uint8_t brightness_percent; /**< Current backlight level set by ui_init(), 5..100%. */
} app_config_t;

/**
 * Returns saved brightness, or the default if storage has no usable value.
 *
 * Call after storage_init(), before ui_init(). Values produced by the brightness
 * screen are 5..100% in steps of 5%; other saved values select the default.
 */
uint8_t app_load_brightness(void);

/**
 * Starts app_task, the only writer of the application state.
 *
 * Call once after attempting ui_init(); the app also runs without a display.
 * The configuration is copied and may be released on return. A failed start
 * releases the input queue and leaves the app unavailable.
 *
 * @param config Non-NULL configuration with brightness in 5..100%.
 * @retval ESP_OK The task was created.
 * @retval ESP_ERR_INVALID_ARG Invalid configuration or brightness.
 * @retval ESP_ERR_INVALID_STATE The app has already started.
 * @retval ESP_ERR_NO_MEM The queue or task could not be allocated.
 */
esp_err_t app_start(const app_config_t* config);

/**
 * Copies a button event into the input queue without waiting for queue space.
 *
 * Safe from the esp_timer task; not an ISR API. Before app_start(), events are
 * ignored. A full queue is reported by app_task, which discards queued input,
 * cancels the shutdown question and ignores leftover gestures until a new press.
 *
 * @param event Non-NULL event; the caller may release it on return.
 */
void app_post_button(const app_button_event_t* event);

/**
 * Notifies app_task to read the latest sensing snapshot without blocking.
 * Safe from the sensing task; not an ISR API. Notifications may coalesce.
 * Before app_start(), the notification is ignored.
 */
void app_notify_measurement(void);
