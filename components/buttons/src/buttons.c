#include "buttons.h"

#include "button_gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "iot_button.h"

static const char* TAG = "buttons";

#define BUTTON_EVENT_QUEUE_LEN 8U

static QueueHandle_t s_event_queue = NULL;
static uint32_t s_event_drop_count = 0;

static void queue_button_event(button_id_t btn_id, bool is_long_press)
{
    if (s_event_queue == NULL) {
        return;
    }

    button_event_msg_t event = {
        .button_id = btn_id,
        .is_long_press = is_long_press,
        .timestamp_us = esp_timer_get_time(),
    };

    if (xQueueSend(s_event_queue, &event, 0) != pdTRUE) {
        s_event_drop_count++;
        if ((s_event_drop_count % 20U) == 1U) {
            ESP_LOGW(TAG, "Button event queue is full, dropping events (%lu)", (unsigned long)s_event_drop_count);
        }
    }
}

static void internal_short_press_cb(void* arg, void* data)
{
    (void)arg;
    int btn_id = (int)(intptr_t)data;
    queue_button_event((button_id_t)btn_id, false);
}

static void internal_long_press_cb(void* arg, void* data)
{
    (void)arg;
    int btn_id = (int)(intptr_t)data;
    queue_button_event((button_id_t)btn_id, true);
}

bool buttons_init(const buttons_config_t* config)
{
    if (!config) {
        ESP_LOGE(TAG, "buttons_init config is NULL");
        return false;
    }

    if (s_event_queue == NULL) {
        s_event_queue = xQueueCreate(BUTTON_EVENT_QUEUE_LEN, sizeof(button_event_msg_t));
        if (s_event_queue == NULL) {
            ESP_LOGE(TAG, "Failed to create button event queue");
            return false;
        }
    }

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
    if (iot_button_new_gpio_device(&btn_cfg, &gpio_cfg_prev, &btn_prev) == ESP_OK) {
        iot_button_register_cb(btn_prev, BUTTON_PRESS_UP, NULL, internal_short_press_cb, (void*)(intptr_t)BTN_ID_PREV);
        iot_button_register_cb(
            btn_prev, BUTTON_LONG_PRESS_START, NULL, internal_long_press_cb, (void*)(intptr_t)BTN_ID_PREV);
        ESP_LOGI(TAG, "PREV button OK");
    } else {
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
    if (iot_button_new_gpio_device(&btn_cfg, &gpio_cfg_next, &btn_next) == ESP_OK) {
        iot_button_register_cb(btn_next, BUTTON_PRESS_UP, NULL, internal_short_press_cb, (void*)(intptr_t)BTN_ID_NEXT);
        iot_button_register_cb(
            btn_next, BUTTON_LONG_PRESS_START, NULL, internal_long_press_cb, (void*)(intptr_t)BTN_ID_NEXT);
        ESP_LOGI(TAG, "NEXT button OK");
    } else {
        ESP_LOGE(TAG, "Failed to create NEXT button");
        all_ok = false;
    }

    return all_ok;
}

bool buttons_get_event(button_event_msg_t* out_event)
{
    if (out_event == NULL || s_event_queue == NULL) {
        return false;
    }

    return xQueueReceive(s_event_queue, out_event, 0) == pdTRUE;
}
