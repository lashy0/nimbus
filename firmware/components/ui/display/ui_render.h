#pragma once

#include "ui.h"

// Creates the renderer, its draw buffers and the shared screen on the SPI core.
esp_err_t ui_render_init(const ui_config_t* config);

// Aborts a failed startup. Removes callbacks and owned resources only after the
// last transfer has finished, then requests asynchronous port shutdown.
// On error resources remain alive because callbacks may still use them.
esp_err_t ui_render_abort_init(void);

esp_err_t ui_render_apply_model(const ui_model_t* model);

// Resumes rendering if stopped, requests a full frame and waits for its transfer.
esp_err_t ui_render_draw_frame(void);

// Stops timers and rendering and keeps the recursive lock until the next frame.
// Repeated calls are allowed only from the task that holds that lock.
esp_err_t ui_render_quiesce(void);
bool ui_render_owned_by_caller(void);
