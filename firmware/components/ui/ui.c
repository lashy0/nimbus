#include "ui.h"

#include "esp_check.h"
#include "esp_log.h"

#include "ui_display.h"
#include "ui_render.h"

static const char* TAG = "ui";
static bool s_ready;
static bool s_attempted;

static void abort_init(void)
{
    const esp_err_t render_err = ui_render_abort_init();
    if (render_err != ESP_OK) {
        ESP_LOGE(TAG, "renderer cleanup failed: %s; resources retained", esp_err_to_name(render_err));
    }

    const esp_err_t display_err = ui_display_abort_init(render_err == ESP_OK);
    if (display_err != ESP_OK) {
        ESP_LOGE(TAG, "display cleanup failed: %s; sleep path must finish shutdown", esp_err_to_name(display_err));
    }
}

esp_err_t ui_init(const ui_config_t* config)
{
    ESP_RETURN_ON_FALSE(
        config != NULL && config->panel_io != NULL && config->panel != NULL && config->set_backlight != NULL &&
            config->release_backlight != NULL && config->reclaim_backlight != NULL && config->brightness_percent <= 100,
        ESP_ERR_INVALID_ARG,
        TAG,
        "config");
    ESP_RETURN_ON_FALSE(!s_attempted, ESP_ERR_INVALID_STATE, TAG, "initialization already attempted");

    // Port shutdown is asynchronous, so a failed startup cannot be retried safely
    // during this boot. Public calls remain unavailable until the first frame.
    s_attempted = true;
    esp_err_t ret = ESP_OK;
    ESP_GOTO_ON_ERROR(ui_display_init(config), fail, TAG, "display");
    ESP_GOTO_ON_ERROR(ui_render_init(config), fail, TAG, "renderer");
    ESP_GOTO_ON_ERROR(ui_display_start(config->brightness_percent), fail, TAG, "start screen");
    s_ready = true;

    return ESP_OK;

fail:
    abort_init();

    return ret;
}

esp_err_t ui_apply_model(const ui_model_t* model)
{
    ESP_RETURN_ON_FALSE(model != NULL, ESP_ERR_INVALID_ARG, TAG, "model");
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    return ui_render_apply_model(model);
}

esp_err_t ui_set_display_state(ui_display_state_t state, uint8_t brightness_percent)
{
    ESP_RETURN_ON_FALSE(state <= UI_DISPLAY_OFF && brightness_percent <= 100, ESP_ERR_INVALID_ARG, TAG, "arguments");
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    return ui_display_set_state(state, brightness_percent);
}
