// Page content: which widgets each screen shows, thresholds, texts and positions.
#include <stdio.h>

#include "ui_internal.h"
#include "ui_widgets.h"

#define TEXT_MAX 32

static const ui_fonts_t* s_fonts;

void ui_view_create(lv_obj_t* screen, const ui_fonts_t* fonts)
{
    s_fonts = fonts;
    ui_widgets_create(screen, fonts);
}

static const char* const CHARGING[] = {
    UI_IMG("batt/1_charging"),
    UI_IMG("batt/2_charging"),
    UI_IMG("batt/3_charging"),
    UI_IMG("batt/full_charging"),
};

#define CHARGING_COUNT (sizeof(CHARGING) / sizeof(CHARGING[0]))

static uint32_t show_battery(const ui_model_t* model)
{
    // The board has no charge signal and the cell is not readable on USB, so the
    // levels cycle with no percentage, as phones show charging.
    if (model->external_power) {
        return ui_show_battery_animation(CHARGING, CHARGING_COUNT, 8);
    }

    if (!model->battery_valid) {
        return ui_show_battery("-- %", NULL, 0);
    }

    static const char* const DISCHARGING[] = {
        UI_IMG("batt/1_not_charging"),
        UI_IMG("batt/2_not_charging"),
        UI_IMG("batt/3_not_charging"),
        UI_IMG("batt/full_not_charging"),
    };
    const uint8_t percent = model->battery_percent;
    const size_t level = percent >= 90 ? 3 : percent >= 60 ? 2 : percent >= 30 ? 1 : 0;

    char text[TEXT_MAX];
    snprintf(text, sizeof(text), "%u %%", percent);

    // The charging icons carry a lightning bolt above the outline and are taller.
    if (model->charging) {
        return ui_show_battery(text, CHARGING[level], 8);
    }

    return ui_show_battery(text, DISCHARGING[level], 11);
}

static uint32_t show_iaq(const ui_model_t* model)
{
    if (model->iaq_state == UI_IAQ_CALIBRATING) {
        return ui_show_title("IAQ", 41, 33) | ui_show_face(UI_FACE_NIMBUS) |
               ui_show_caption("Калибровка\nсенсора", 174) | ui_show_dots(0);
    }

    const uint32_t visible = ui_show_title("IAQ", 13, 33) | ui_show_dots(0);
    if (model->iaq_state == UI_IAQ_NONE) {
        return visible | ui_show_value("---", s_fonts->digits_60) | ui_show_face(UI_FACE_ORDINARY);
    }

    static const ui_face_t FACE_BY_LEVEL[] = {
        UI_FACE_ULTRA_HAPPY,
        UI_FACE_HAPPY,
        UI_FACE_ORDINARY,
        UI_FACE_SAD,
        UI_FACE_DIZZY,
        UI_FACE_DEAD,
    };
    static const char* const PLATE_BY_LEVEL[] = {
        UI_IMG("status/good"),
        UI_IMG("status/good"),
        UI_IMG("status/mid"),
        UI_IMG("status/bad"),
        UI_IMG("status/warn"),
        UI_IMG("status/crit"),
    };
    const uint16_t iaq = model->iaq;
    const size_t level = iaq <= 50 ? 0 : iaq <= 100 ? 1 : iaq <= 200 ? 2 : iaq <= 300 ? 3 : iaq <= 400 ? 4 : 5;

    char text[TEXT_MAX];
    snprintf(text, sizeof(text), "%03u", iaq);

    return visible | ui_show_value(text, s_fonts->digits_60) | ui_show_plate(PLATE_BY_LEVEL[level], 67, 34) |
           ui_show_face(FACE_BY_LEVEL[level]);
}

static uint32_t show_temperature(const ui_model_t* model)
{
    const uint32_t visible = ui_show_title("Temp", 20, 34) | ui_show_dots(1);
    if (!model->temperature_valid) {
        return visible | ui_show_value("--°", s_fonts->digits_60) | ui_show_face(UI_FACE_TEMP_NORMAL);
    }

    const int16_t celsius = model->temperature_c;
    char text[TEXT_MAX];
    snprintf(text, sizeof(text), "%d°", celsius);

    uint32_t mark;
    ui_face_t face;
    if (celsius < 0) {
        mark = ui_show_plate(UI_IMG("status/minus"), 97, 41);
        face = UI_FACE_TEMP_MINUS;
    } else if (celsius <= 25) {
        mark = ui_show_plate(UI_IMG("status/nothing"), 99, 41);
        face = UI_FACE_TEMP_NORMAL;
    } else {
        mark = ui_show_plate(UI_IMG("status/plus"), 97, 41);
        face = UI_FACE_TEMP_PLUS;
    }

    return visible | mark | ui_show_value(text, s_fonts->digits_60) | ui_show_face(face);
}

static uint32_t show_humidity(const ui_model_t* model)
{
    const uint32_t visible = ui_show_title("Hum", 4, 34) | ui_show_dots(2);
    if (!model->humidity_valid) {
        return visible | ui_show_value("--%", s_fonts->digits_60) | ui_show_face(UI_FACE_ORDINARY);
    }

    const uint8_t percent = model->humidity_percent;
    char text[TEXT_MAX];
    snprintf(text, sizeof(text), "%u%%", percent);

    // "100%" does not fit the screen width in the 60 px font.
    const lv_font_t* font = percent >= 100 ? s_fonts->digits_50 : s_fonts->digits_60;

    uint32_t plate;
    ui_face_t face;
    if (percent < 30) {
        plate = ui_show_plate(UI_IMG("status/dry"), 73, 34);
        face = UI_FACE_ORDINARY;
    } else if (percent < 70) {
        plate = ui_show_plate(UI_IMG("status/good"), 73, 34);
        face = UI_FACE_HAPPY;
    } else {
        plate = ui_show_plate(UI_IMG("status/damp"), 73, 34);
        face = UI_FACE_DIVER;
    }

    return visible | plate | ui_show_value(text, font) | ui_show_face(face);
}

static uint32_t show_brightness(const ui_model_t* model)
{
    char text[TEXT_MAX];
    snprintf(text, sizeof(text), "Яркость\n%u%%", model->brightness_percent);

    return UI_SHOW(UI_W_SUN) | ui_show_caption(text, 125) | ui_show_bar(model->brightness_percent);
}

static uint32_t show_shutdown(const ui_model_t* model)
{
    return ui_show_face(UI_FACE_HUH) | ui_show_caption("Выключить\nустройство?", 140) |
           ui_show_answers(model->shutdown_yes_selected);
}

void ui_view_apply(const ui_model_t* model)
{
    uint32_t visible = 0;
    switch (model->screen) {
        case UI_SCREEN_START:
            visible = ui_show_face(UI_FACE_BASE);
            break;
        case UI_SCREEN_IAQ:
            visible = show_iaq(model);
            break;
        case UI_SCREEN_TEMP:
            visible = show_temperature(model);
            break;
        case UI_SCREEN_HUM:
            visible = show_humidity(model);
            break;
        case UI_SCREEN_BRIGHTNESS:
            visible = show_brightness(model);
            break;
        case UI_SCREEN_SHUTDOWN:
            visible = show_shutdown(model);
            break;
    }

    if (model->screen != UI_SCREEN_START) {
        visible |= show_battery(model);
    }

    // The USB notice covers the page without changing it.
    if (model->usb_notice) {
        visible = ui_show_lightning();
    }

    ui_widgets_set_visible(visible);
}
