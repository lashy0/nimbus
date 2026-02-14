#include "app.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_timer.h"
#include "ui.h"
#include "power_manager.h"

static const char* TAG = "app";
static const int64_t APP_IDLE_TIMEOUT_US = 60LL * 1000000LL;

static app_config_t app_cfg;
static button_id_t question_activated_by = BTN_ID_NONE;
static button_id_t ignore_next_short_for = BTN_ID_NONE;
static int64_t last_activity_time_us = 0;

static void app_mark_activity(void)
{
    last_activity_time_us = esp_timer_get_time();
}

static void on_shutdown_yes(void)
{
    power_manager_shutdown();
}

static void on_shutdown_no(void)
{
    ESP_LOGI(TAG, "Shutdown cancelled");
    question_activated_by = BTN_ID_NONE;

    if (lvgl_port_lock(100)) {
        ui_hide_special();
        lvgl_port_unlock();
    } else {
        ESP_LOGW(TAG, "Failed to lock LVGL to close shutdown question");
    }
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
    app_mark_activity();
}

void app_on_button_short_press(button_id_t btn_id)
{
    if (!lvgl_port_lock(10)) return;

    if (power_manager_is_monitoring()) {
        ESP_LOGI(TAG, "Wake display from monitoring mode");
        power_manager_exit_monitoring();
        app_mark_activity();
        lvgl_port_unlock();
        return;
    }

    enum ScreensEnum current = ui_get_current_screen();

    if (current == SCREEN_ID_START || current == SCREEN_ID_CALIBRATION)
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
            question_activated_by = BTN_ID_NONE;
            app_mark_activity();
            lvgl_port_unlock();
            ui_question_confirm();
            return;
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

    app_mark_activity();
    lvgl_port_unlock();
}

void app_on_button_long_press(button_id_t btn_id)
{
    if (!lvgl_port_lock(10)) return;

    if (power_manager_is_monitoring()) {
        ESP_LOGI(TAG, "Wake display from monitoring mode (long press)");
        power_manager_exit_monitoring();
        app_mark_activity();
        lvgl_port_unlock();
        return;
    }

    enum ScreensEnum current = ui_get_current_screen();

    if (current == SCREEN_ID_START || current == SCREEN_ID_CALIBRATION || current == SCREEN_ID_QUESTION) {
        lvgl_port_unlock();
        return;
    }

    ESP_LOGI(TAG, "Long press - showing shutdown question");

    question_activated_by = btn_id;
    ignore_next_short_for = btn_id;
    bool select_yes = (btn_id == BTN_ID_PREV);
    ui_show_question("Turn off\nthe device?", on_shutdown_yes, on_shutdown_no, select_yes);

    app_mark_activity();
    lvgl_port_unlock();
}

void app_process_idle(void)
{
    if (power_manager_is_monitoring()) {
        return;
    }

    if (last_activity_time_us == 0) {
        app_mark_activity();
        return;
    }

    int64_t now = esp_timer_get_time();
    if ((now - last_activity_time_us) >= APP_IDLE_TIMEOUT_US) {
        ESP_LOGI(TAG, "Idle timeout reached -> monitoring mode");
        power_manager_enter_monitoring();
    }
}
