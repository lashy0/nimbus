#include <stdbool.h>
#include <time.h>

#include "lvgl.h"

#include "sim_data.h"
#include "sim_input.h"
#include "sim_log.h"
#include "sim_port.h"
#include "ui_app.h"
#include "ui_view.h"

#define DISPLAY_WIDTH 135
#define DISPLAY_HEIGHT 240
#define ZOOM 2

// The periods of the firmware UI.
#define SENSING_PERIOD_MS 1000
#define POWER_PERIOD_MS 250
#define BUTTONS_PERIOD_MS 20

static bool s_powered_off;

static void on_sensing(lv_timer_t* timer)
{
    sensing_snapshot_t snapshot;
    sim_data_get_snapshot(&snapshot);
    ui_view_update(&snapshot);
}

static void on_power(lv_timer_t* timer)
{
    power_status_t status;
    sim_data_get_power(&status);
    ui_app_update_power(&status);
}

static void on_buttons(lv_timer_t* timer)
{
    if (s_powered_off) {
        return;
    }

    buttons_event_t event;
    while (sim_input_receive(&event)) {
        ui_app_press(&event);
    }

    if (ui_view_power_off_confirmed()) {
        s_powered_off = true;
        sim_port_power_off();
    }
}

int main(void)
{
    lv_init();
    sim_log_start();
    lv_rand_set_seed((uint32_t)time(NULL));

    lv_display_t* display = lv_sdl_window_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_sdl_window_set_zoom(display, ZOOM);
    lv_sdl_window_set_title(display, "Nimbus");

    sim_port_create();
    ui_app_create(0);
    lv_timer_create(on_sensing, SENSING_PERIOD_MS, NULL);
    lv_timer_create(on_power, POWER_PERIOD_MS, NULL);
    lv_timer_create(on_buttons, BUTTONS_PERIOD_MS, NULL);
    sim_input_start();

    for (;;) {
        lv_delay_ms(lv_timer_handler());
    }
}
