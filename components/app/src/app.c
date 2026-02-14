#include "app.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "ui.h"
#include "power_manager.h"

static const char* TAG = "app";

static app_config_t app_cfg;
static button_id_t question_activated_by = BTN_ID_NONE;
static button_id_t ignore_next_short_for = BTN_ID_NONE;

static void on_shutdown_yes(void)
{
    power_manager_shutdown();
}

static void on_shutdown_no(void)
{
    ESP_LOGI(TAG, "Shutdown cancelled");
    question_activated_by = BTN_ID_NONE;
    ui_hide_special();
}

void app_init(const app_config_t* config)
{
    app_cfg = *config;

    power_manager_config_t pm_cfg = {
        .panel_handle = config->display->panel_handle,
        .bl_handle = config->backlight,
    };
    power_manager_init(&pm_cfg);

    power_manager_check_wakeup_reason();
}

void app_on_button_short_press(button_id_t btn_id)
{
    if (!lvgl_port_lock(10)) return;

    enum ScreensEnum current = ui_get_current_screen();

    if (current == SCREEN_ID_START)
    {
        lvgl_port_unlock();
        return;
    }

    if (current == SCREEN_ID_QUESTION)
    {
        if (btn_id == ignore_next_short_for)
        {
            ESP_LOGI(TAG, "Ignoring short press on release after long press");
            ignore_next_short_for = BTN_ID_NONE;
            lvgl_port_unlock();
            return;
        }

        if (btn_id == question_activated_by)
        {
            ESP_LOGI(TAG, "Question confirmed");
            ui_question_confirm();
            question_activated_by = BTN_ID_NONE;
        }
        else
        {
            ESP_LOGI(TAG, "Question selection changed");
            if (btn_id == BTN_ID_PREV) {
                ui_question_select_yes();
                question_activated_by = BTN_ID_PREV;
            } else {
                ui_question_select_no();
                question_activated_by = BTN_ID_NEXT;
            }
        }
    }
    else
    {
        if (btn_id == BTN_ID_PREV) {
            ESP_LOGI(TAG, "Switch to prev screen");
            ui_switch_prev();
        } else {
            ESP_LOGI(TAG, "Switch to next screen");
            ui_switch_next();
        }
    }

    lvgl_port_unlock();
}

void app_on_button_long_press(button_id_t btn_id)
{
    if (!lvgl_port_lock(10)) return;

    enum ScreensEnum current = ui_get_current_screen();

    if (current == SCREEN_ID_START || current == SCREEN_ID_QUESTION) {
        lvgl_port_unlock();
        return;
    }

    ESP_LOGI(TAG, "Long press - showing shutdown question");

    question_activated_by = btn_id;
    ignore_next_short_for = btn_id;
    bool select_yes = (btn_id == BTN_ID_PREV);
    ui_show_question("Turn off\nthe device?", on_shutdown_yes, on_shutdown_no, select_yes);

    lvgl_port_unlock();
}
