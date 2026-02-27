#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Logical IDs of supported board buttons.
 */
typedef enum { BTN_ID_NONE = -1, BTN_ID_PREV = 0, BTN_ID_NEXT = 1 } button_id_t;

/**
 * @brief Callback type for short-press events.
 *
 * @param[in] btn_id Logical button identifier.
 */
typedef void (*button_short_press_cb_t)(button_id_t btn_id);

/**
 * @brief Callback type for long-press events.
 *
 * @param[in] btn_id Logical button identifier.
 */
typedef void (*button_long_press_cb_t)(button_id_t btn_id);

/**
 * @brief Button event message fetched from queue.
 */
typedef struct {
    /**< Logical button identifier. */
    button_id_t button_id;
    /**< True for long-press event, false for short-press event. */
    bool is_long_press;
    /**< Event timestamp in microseconds. */
    int64_t timestamp_us;
} button_event_msg_t;

/**
 * @brief GPIO and timing configuration for button handling.
 */
typedef struct {
    /**< GPIO number for PREV button. */
    gpio_num_t prev_gpio;
    /**< GPIO number for NEXT button. */
    gpio_num_t next_gpio;
    /**< Long-press threshold in milliseconds. */
    uint16_t long_press_time_ms;
    /**< Minimal short-press time in milliseconds. */
    uint16_t short_press_time_ms;
    /**< Callback for short-press events. */
    button_short_press_cb_t on_short_press;
    /**< Callback for long-press events. */
    button_long_press_cb_t on_long_press;
} buttons_config_t;

/**
 * @brief Initialize button drivers and event callbacks.
 *
 * @param[in] config Pointer to button configuration.
 *
 * @return
 * - true: initialization succeeded.
 * - false: initialization failed.
 */
bool buttons_init(const buttons_config_t* config);

/**
 * @brief Fetch the next button event from internal queue.
 *
 * @param[out] out_event Output pointer for event data.
 *
 * @return
 * - true: event received.
 * - false: queue is empty or unavailable.
 */
bool buttons_get_event(button_event_msg_t* out_event);

#ifdef __cplusplus
}
#endif
