#include "ui_internal.h"

#include "images.h"

int current_iaq = 0;
int current_temp = 0;
int current_hum = 0;
int current_batt_pct = -1;
bool current_batt_charging = false;
bool current_stabilization_done = false;
bool current_run_in_done = false;
uint8_t current_brightness_pct = 60;
const uint8_t UI_BRIGHTNESS_MIN_PCT = 5;

enum ScreensEnum currentScreenId = SCREEN_ID_NONE;
enum ScreensEnum previousScreenId = SCREEN_ID_NONE;
bool startup_non_critical_error = false;
lv_obj_t* startup_error_icon = NULL;

void (*question_on_yes)(void) = NULL;
void (*question_on_no)(void) = NULL;
bool question_selected_yes = true;

static void ui_show_startup_error_indicator(void)
{
    if (!startup_non_critical_error || startup_error_icon) {
        return;
    }

    startup_error_icon = lv_img_create(lv_layer_top());
    img_set(startup_error_icon, &IMG_INFO_DEAD);
    lv_obj_set_pos(startup_error_icon, 2, 2);
    lv_obj_clear_flag(startup_error_icon, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
}

enum ScreensEnum ui_get_current_screen(void)
{
    return currentScreenId;
}

void ui_init(void)
{
    ui_show_start();
}

void ui_finish_startup(bool has_non_critical_error)
{
    startup_non_critical_error = has_non_critical_error;

    if (startup_non_critical_error) {
        ui_show_no_charging();
    } else if (currentScreenId == SCREEN_ID_START) {
        loadScreen(SCREEN_ID_IAQ);
    }

    ui_show_startup_error_indicator();
}

void ui_tick(void) {}
