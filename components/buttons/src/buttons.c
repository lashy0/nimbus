#include "buttons.h"

#include "button_gpio.h"
#include "esp_log.h"
#include "iot_button.h"

static const char* TAG = "buttons";

static buttons_config_t btn_config;

static void internal_short_press_cb(void* arg, void* data)
{
    int btn_id = (int)(intptr_t)data;
    if (btn_config.on_short_press)
    {
        btn_config.on_short_press((button_id_t)btn_id);
    }
}

static void internal_long_press_cb(void* arg, void* data)
{
    int btn_id = (int)(intptr_t)data;
    if (btn_config.on_long_press)
    {
        btn_config.on_long_press((button_id_t)btn_id);
    }
}

bool buttons_init(const buttons_config_t* config)
{
    if (!config) {
        ESP_LOGE(TAG, "buttons_init config is NULL");
        return false;
    }

    btn_config = *config;
    bool all_ok = true;

    const button_config_t btn_cfg = {
        .long_press_time = config->long_press_time_ms,
        .short_press_time = config->short_press_time_ms,
    };

    // PREV button
    button_gpio_config_t gpio_cfg_prev = {
        .gpio_num = config->prev_gpio,
        .active_level = 0,
    };

    button_handle_t btn_prev = NULL;
    ESP_LOGI(TAG, "Creating PREV button on GPIO %d", config->prev_gpio);
    if (iot_button_new_gpio_device(&btn_cfg, &gpio_cfg_prev, &btn_prev) == ESP_OK)
    {
        iot_button_register_cb(
            btn_prev, BUTTON_PRESS_UP, NULL, internal_short_press_cb, (void*)(intptr_t)BTN_ID_PREV
        );
        iot_button_register_cb(
            btn_prev, BUTTON_LONG_PRESS_START, NULL, internal_long_press_cb, (void*)(intptr_t)BTN_ID_PREV
        );
        ESP_LOGI(TAG, "PREV button OK");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to create PREV button");
        all_ok = false;
    }

    // NEXT button
    button_gpio_config_t gpio_cfg_next = {
        .gpio_num = config->next_gpio,
        .active_level = 0,
        .disable_pull = true,
    };

    button_handle_t btn_next = NULL;
    ESP_LOGI(TAG, "Creating NEXT button on GPIO %d", config->next_gpio);
    if (iot_button_new_gpio_device(&btn_cfg, &gpio_cfg_next, &btn_next) == ESP_OK)
    {
        iot_button_register_cb(
            btn_next, BUTTON_PRESS_UP, NULL, internal_short_press_cb, (void*)(intptr_t)BTN_ID_NEXT
        );
        iot_button_register_cb(
            btn_next, BUTTON_LONG_PRESS_START, NULL, internal_long_press_cb, (void*)(intptr_t)BTN_ID_NEXT);
        ESP_LOGI(TAG, "NEXT button OK");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to create NEXT button");
        all_ok = false;
    }

    return all_ok;
}
