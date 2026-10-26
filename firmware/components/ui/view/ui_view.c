#include "ui_view.h"

#include <stdio.h>

#include "esp_check.h"
#include "ui_animation.h"
#include "ui_assets.h"
#include "ui_faces.h"
#include "ui_widgets.h"

#define TEXT_MAX 32

typedef struct {
    uint8_t min_percent;
    const char* icon;
} battery_level_t;

typedef struct {
    uint16_t max_iaq;
    ui_face_t face;
    const char* plate;
} iaq_level_t;

static const battery_level_t BATTERY_LEVELS[] = {
    {0, UI_IMAGE_BATTERY_LOW},
    {30, UI_IMAGE_BATTERY_MID},
    {60, UI_IMAGE_BATTERY_HIGH},
    {90, UI_IMAGE_BATTERY_FULL},
};

static const iaq_level_t IAQ_LEVELS[] = {
    {50, UI_FACE_ULTRA_HAPPY, UI_IMAGE_STATUS_GOOD},
    {100, UI_FACE_HAPPY, UI_IMAGE_STATUS_GOOD},
    {200, UI_FACE_ORDINARY, UI_IMAGE_STATUS_MID},
    {300, UI_FACE_SAD, UI_IMAGE_STATUS_BAD},
    {400, UI_FACE_DIZZY, UI_IMAGE_STATUS_WARN},
    {500, UI_FACE_DEAD, UI_IMAGE_STATUS_CRIT},
};

#define BATTERY_LEVEL_COUNT (sizeof(BATTERY_LEVELS) / sizeof(BATTERY_LEVELS[0]))
#define IAQ_LEVEL_COUNT (sizeof(IAQ_LEVELS) / sizeof(IAQ_LEVELS[0]))

static const char* TAG = "ui_view";
static const ui_fonts_t* s_fonts;

esp_err_t ui_view_create(lv_obj_t* screen, const ui_fonts_t* fonts)
{
    s_fonts = fonts;
    ESP_RETURN_ON_ERROR(ui_widgets_create(screen, fonts), TAG, "widgets");

    return ESP_OK;
}

void ui_view_deinit(void)
{
    ui_blink_deinit();
    ui_charge_deinit();
    s_fonts = NULL;
}

static size_t battery_level_index(uint8_t percent)
{
    for (size_t i = BATTERY_LEVEL_COUNT - 1; i > 0; i--) {
        if (percent >= BATTERY_LEVELS[i].min_percent) {
            return i;
        }
    }

    return 0;
}

static const iaq_level_t* iaq_level(uint16_t iaq)
{
    for (size_t i = 0; i < IAQ_LEVEL_COUNT - 1; i++) {
        if (iaq <= IAQ_LEVELS[i].max_iaq) {
            return &IAQ_LEVELS[i];
        }
    }

    return &IAQ_LEVELS[IAQ_LEVEL_COUNT - 1];
}

static uint32_t show_battery(const ui_battery_t* battery)
{
    switch (battery->state) {
        case UI_BATTERY_EXTERNAL_POWER:
            // The board has no charge signal and the cell is not readable on USB.
            return ui_show_battery_animation(UI_BATTERY_CHARGING, UI_BATTERY_CHARGING_COUNT, 8);
        case UI_BATTERY_UNKNOWN:
            return ui_show_battery("-- %", NULL, 0);
        case UI_BATTERY_LEVEL:
            break;
    }

    const uint8_t percent = battery->percent;
    const size_t level = battery_level_index(percent);

    char text[TEXT_MAX];
    snprintf(text, sizeof(text), "%u %%", percent);

    return ui_show_battery(text, BATTERY_LEVELS[level].icon, 11);
}

static uint32_t show_iaq(const ui_iaq_t* measurement)
{
    if (measurement->calibrating) {
        return ui_show_title("IAQ", 41, 33) | ui_show_face(UI_FACE_NIMBUS) |
               ui_show_caption("Калибровка\nсенсора", 174) | ui_show_dots(0);
    }

    const uint16_t iaq = measurement->value;
    const iaq_level_t* level = iaq_level(iaq);

    char text[TEXT_MAX];
    snprintf(text, sizeof(text), "%03u", iaq);

    return ui_show_title("IAQ", 13, 33) | ui_show_dots(0) | ui_show_value(text, s_fonts->digits_60) |
           ui_show_plate(level->plate, 67, 34) | ui_show_face(level->face);
}

static uint32_t show_temperature(int16_t celsius)
{
    const uint32_t visible = ui_show_title("Temp", 17, 34) | ui_show_dots(1);
    char text[TEXT_MAX];
    snprintf(text, sizeof(text), "%d°", celsius);

    uint32_t mark;
    ui_face_t face;
    if (celsius < 0) {
        mark = ui_show_plate(UI_IMAGE_STATUS_MINUS, 97, 41);
        face = UI_FACE_TEMP_MINUS;
    } else if (celsius <= 25) {
        mark = ui_show_plate(UI_IMAGE_STATUS_NOTHING, 99, 41);
        face = UI_FACE_TEMP_NORMAL;
    } else {
        mark = ui_show_plate(UI_IMAGE_STATUS_PLUS, 97, 41);
        face = UI_FACE_TEMP_PLUS;
    }

    return visible | mark | ui_show_value(text, s_fonts->digits_60) | ui_show_face(face);
}

static uint32_t show_humidity(uint8_t percent)
{
    const uint32_t visible = ui_show_title("Hum", 4, 34) | ui_show_dots(2);
    char text[TEXT_MAX];
    snprintf(text, sizeof(text), "%u%%", percent);

    // "100%" does not fit the screen width in the 60 px font.
    const lv_font_t* font = percent >= 100 ? s_fonts->digits_50 : s_fonts->digits_60;

    uint32_t plate;
    ui_face_t face;
    if (percent < 30) {
        plate = ui_show_plate(UI_IMAGE_STATUS_DRY, 73, 34);
        face = UI_FACE_ORDINARY;
    } else if (percent < 70) {
        plate = ui_show_plate(UI_IMAGE_STATUS_GOOD, 73, 34);
        face = UI_FACE_HAPPY;
    } else {
        plate = ui_show_plate(UI_IMAGE_STATUS_DAMP, 73, 34);
        face = UI_FACE_DIVER;
    }

    return visible | plate | ui_show_value(text, font) | ui_show_face(face);
}

// One page for all three measurements: they come from the same sensor.
static uint32_t show_sensor_error(void)
{
    return ui_show_face(UI_FACE_DIZZY_NIMBUS) | ui_show_caption("Ошибка\nсенсора", 140);
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
            visible = show_iaq(&model->measurements.iaq);
            break;
        case UI_SCREEN_TEMP:
            visible = show_temperature(model->measurements.temperature_c);
            break;
        case UI_SCREEN_HUM:
            visible = show_humidity(model->measurements.humidity_percent);
            break;
        case UI_SCREEN_SENSOR_ERROR:
            visible = show_sensor_error();
            break;
        case UI_SCREEN_BRIGHTNESS:
            visible = show_brightness(model);
            break;
        case UI_SCREEN_SHUTDOWN:
            visible = show_shutdown(model);
            break;
    }

    if (model->screen != UI_SCREEN_START) {
        visible |= show_battery(&model->battery);
    }

    // The USB notice covers the page without changing it.
    if (model->usb_notice) {
        visible = ui_show_lightning();
    }

    ui_widgets_set_visible(visible);
}
