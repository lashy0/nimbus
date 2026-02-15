#pragma once

#include "buttons.h"
#include "backlight.h"
#include "display.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Application initialization dependencies.
 */
typedef struct {
    /**< Display hardware handles managed by the application. */
    display_handles_t* display;
    /**< Backlight handle used by the application for runtime control. */
    backlight_handle_t* backlight;
} app_config_t;

/**
 * @brief Initialize application logic and internal state.
 *
 * @param[in] config Pointer to application dependencies.
 */
void app_init(const app_config_t* config);

/**
 * @brief Handle short-press event from a button.
 *
 * @param[in] btn_id Logical button identifier.
 */
void app_on_button_short_press(button_id_t btn_id);

/**
 * @brief Handle long-press event from a button.
 *
 * @param[in] btn_id Logical button identifier.
 */
void app_on_button_long_press(button_id_t btn_id);

/**
 * @brief Process idle-related application tasks.
 *
 * Intended to be called periodically from the main runtime loop/timer.
 */
void app_process_idle(void);

#ifdef __cplusplus
}
#endif
