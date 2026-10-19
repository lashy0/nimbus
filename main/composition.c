#include "composition.h"

#include "board.h"
#include "board_buttons.h"
#include "esp_check.h"
#include "esp_log.h"

static const char* TAG = "composition";

// Until app exists, button events are only logged; nothing acts on them.
static void log_button_event(void* ctx, const board_button_event_t* event)
{
    static const char* const BUTTONS[BOARD_BUTTON_COUNT] = {"PREV", "NEXT"};
    static const char* const TYPES[] = {"PRESSED", "LONG", "SHORT", "RELEASED"};

    ESP_LOGI(TAG, "button %s %s t=%lld us", BUTTONS[event->button], TYPES[event->type], event->timestamp_us);
}

esp_err_t composition_init(void)
{
    ESP_RETURN_ON_ERROR(board_init_safe_gpio(), TAG, "safe GPIO");

    return board_buttons_init(log_button_event, NULL);
}
