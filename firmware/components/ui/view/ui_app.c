#include "ui_app.h"

#include "lvgl.h"

#include "ui_port.h"
#include "ui_view.h"

#define SAVE_DELAY_MS 2000 // After the last change.
#define DIM_AFTER_MS 30000 // Without presses.
#define OFF_AFTER_MS 60000
#define DIM_PERCENT_MAX 10

typedef enum {
    SCREEN_ON,
    SCREEN_DIM,
    SCREEN_OFF,
} screen_t;

static screen_t s_screen;
static uint8_t s_brightness; // Chosen on the brightness page.
static lv_timer_t* s_save_timer;
static lv_timer_t* s_idle_timer;
static screen_t s_after_notice = SCREEN_ON; // Screen the USB notice lit; SCREEN_ON when none.

static uint8_t backlight_percent(void)
{
    switch (s_screen) {
        case SCREEN_ON:
            return s_brightness;
        case SCREEN_DIM:
            return s_brightness < DIM_PERCENT_MAX ? s_brightness : DIM_PERCENT_MAX;
        case SCREEN_OFF:
            break;
    }

    return 0;
}

static void on_save(lv_timer_t* timer)
{
    lv_timer_pause(timer);
    ui_port_save_brightness(s_brightness);
}

static void set_screen(screen_t screen)
{
    if (screen == s_screen) {
        return;
    }

    s_screen = screen;
    ui_port_set_backlight(backlight_percent());
    if (screen == SCREEN_OFF) {
        ui_view_close_dialogs();
        ui_app_save_pending();
    }
}

static void on_idle(lv_timer_t* timer)
{
    if (s_screen == SCREEN_ON) {
        set_screen(SCREEN_DIM);
        lv_timer_set_period(timer, OFF_AFTER_MS - DIM_AFTER_MS);
    } else {
        set_screen(SCREEN_OFF);
        lv_timer_pause(timer);
    }
}

static void restart_idle(void)
{
    lv_timer_set_period(s_idle_timer, DIM_AFTER_MS);
    lv_timer_reset(s_idle_timer);
    lv_timer_resume(s_idle_timer);
}

static void on_notice_closed(void)
{
    if (s_after_notice != SCREEN_ON) {
        set_screen(s_after_notice);
        s_after_notice = SCREEN_ON;
    }
}

void ui_app_create(uint8_t brightness)
{
    ui_view_create(brightness, on_notice_closed);
    s_brightness = ui_view_brightness();

    s_save_timer = lv_timer_create(on_save, SAVE_DELAY_MS, NULL);
    lv_timer_pause(s_save_timer);
    s_idle_timer = lv_timer_create(on_idle, DIM_AFTER_MS, NULL);

    ui_port_set_backlight(backlight_percent());
}

void ui_app_press(const buttons_event_t* event)
{
    restart_idle();
    s_after_notice = SCREEN_ON;

    // A press on a dim or dark screen only brings it back.
    if (s_screen != SCREEN_ON) {
        set_screen(SCREEN_ON);

        return;
    }

    ui_view_press(event);

    const uint8_t brightness = ui_view_brightness();
    if (brightness != s_brightness) {
        s_brightness = brightness;
        ui_port_set_backlight(backlight_percent());
        lv_timer_reset(s_save_timer);
        lv_timer_resume(s_save_timer);
    }
}

void ui_app_update_power(const power_status_t* status)
{
    const bool was_shown = ui_view_usb_notice_shown();
    ui_view_update_power(status);
    if (ui_view_usb_notice_shown() && !was_shown && s_screen != SCREEN_ON) {
        s_after_notice = s_screen;
        set_screen(SCREEN_ON);
    }
}

void ui_app_save_pending(void)
{
    if (!lv_timer_get_paused(s_save_timer)) {
        on_save(s_save_timer);
    }
}
