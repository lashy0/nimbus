#include "buttons.h"

#include <stdint.h>

#include "button_gpio.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "iot_button.h"
#include "sdkconfig.h"

#define LONG_PRESS_MS 1500
#define HOLD_STEP_MS CONFIG_BUTTON_LONG_PRESS_HOLD_SERIAL_TIME_MS
#define REPEAT_WINDOW_MS 50
#define QUEUE_LENGTH 8
#define BUTTON_COUNT 2

static const char* TAG = "buttons";

static QueueHandle_t s_queue;
static bool s_long_sent[BUTTON_COUNT]; // Used only in the esp_timer task.

static void send(buttons_id_t id, buttons_press_t press)
{
    const buttons_event_t event = {.id = id, .press = press};
    xQueueSend(s_queue, &event, 0);
}

static void on_press_down(void* handle, void* arg)
{
    s_long_sent[(intptr_t)arg] = false;
}

static void on_hold(void* handle, void* arg)
{
    const buttons_id_t id = (buttons_id_t)(intptr_t)arg;
    if (!s_long_sent[id]) {
        s_long_sent[id] = true;
        send(id, BUTTONS_PRESS_LONG);
    }
}

static void on_press_up(void* handle, void* arg)
{
    const buttons_id_t id = (buttons_id_t)(intptr_t)arg;
    if (!s_long_sent[id]) {
        send(id, BUTTONS_PRESS_SHORT);
    }
}

static esp_err_t create_button(gpio_num_t pin, buttons_id_t id)
{
    const button_config_t config = {
        .long_press_time = LONG_PRESS_MS - HOLD_STEP_MS,
        .short_press_time = REPEAT_WINDOW_MS,
    };
    const button_gpio_config_t gpio_config = {
        .gpio_num = pin,
        .active_level = 0,
        .disable_pull = !GPIO_IS_VALID_OUTPUT_GPIO(pin), // Input-only pins have no internal pull.
        .enable_power_save = true,
    };

    button_handle_t button;
    ESP_RETURN_ON_ERROR(iot_button_new_gpio_device(&config, &gpio_config, &button), TAG, "GPIO %d", pin);

    // Not LONG_PRESS_START: it reads past its callback array.
    void* arg = (void*)(intptr_t)id;
    ESP_RETURN_ON_ERROR(iot_button_register_cb(button, BUTTON_PRESS_DOWN, NULL, on_press_down, arg), TAG, "down");
    ESP_RETURN_ON_ERROR(iot_button_register_cb(button, BUTTON_LONG_PRESS_HOLD, NULL, on_hold, arg), TAG, "hold");

    return iot_button_register_cb(button, BUTTON_PRESS_UP, NULL, on_press_up, arg);
}

esp_err_t buttons_start(const buttons_config_t* config)
{
    ESP_RETURN_ON_FALSE(s_queue == NULL, ESP_ERR_INVALID_STATE, TAG, "already started");

    s_queue = xQueueCreate(QUEUE_LENGTH, sizeof(buttons_event_t));
    ESP_RETURN_ON_FALSE(s_queue != NULL, ESP_ERR_NO_MEM, TAG, "queue");

    ESP_RETURN_ON_ERROR(create_button(config->prev, BUTTONS_PREV), TAG, "prev");

    return create_button(config->next, BUTTONS_NEXT);
}

bool buttons_receive(buttons_event_t* out)
{
    return s_queue != NULL && xQueueReceive(s_queue, out, 0) == pdTRUE;
}

bool buttons_wait(TickType_t timeout)
{
    if (s_queue == NULL) {
        vTaskDelay(timeout);

        return false;
    }

    buttons_event_t event;

    return xQueuePeek(s_queue, &event, timeout) == pdTRUE;
}
