#include "ui_widgets.h"

#include <string.h>

#define COLOR_ACCENT 0xff6600
#define COLOR_INACTIVE_DOT 0x808080
#define COLOR_BAR_TRACK 0x9a9a9a

#define SCREEN_WIDTH 135
#define DOT_COUNT 3
#define DOT_SIZE 5
#define DOT_STEP 15

static lv_obj_t* s_widgets[UI_W_COUNT];
static lv_obj_t* s_yes;
static lv_obj_t* s_no;
static lv_obj_t* s_dots[DOT_COUNT];

static void set_text(lv_obj_t* label, const char* text)
{
    if (strcmp(lv_label_get_text(label), text) != 0) {
        lv_label_set_text(label, text);
    }
}

static void set_font(lv_obj_t* label, const lv_font_t* font)
{
    if (lv_obj_get_style_text_font(label, LV_PART_MAIN) != font) {
        lv_obj_set_style_text_font(label, font, 0);
    }
}

static void set_pos(lv_obj_t* obj, int32_t x, int32_t y)
{
    if (lv_obj_get_style_x(obj, LV_PART_MAIN) != x || lv_obj_get_style_y(obj, LV_PART_MAIN) != y) {
        lv_obj_set_pos(obj, x, y);
    }
}

// The image keeps its own copy of a file path, so compare contents, not pointers.
static void set_src(lv_obj_t* image, const char* src)
{
    const char* current = lv_image_get_src(image);
    if (current == NULL || strcmp(current, src) != 0) {
        lv_image_set_src(image, src);
    }
}

static void set_bg(lv_obj_t* obj, lv_color_t color, lv_opa_t opa)
{
    if (!lv_color_eq(lv_obj_get_style_bg_color(obj, LV_PART_MAIN), color)) {
        lv_obj_set_style_bg_color(obj, color, 0);
    }

    if (lv_obj_get_style_bg_opa(obj, LV_PART_MAIN) != opa) {
        lv_obj_set_style_bg_opa(obj, opa, 0);
    }
}

static void set_visible(lv_obj_t* obj, bool visible)
{
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN) != visible) {
        return;
    }

    if (visible) {
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static lv_obj_t* create_box(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t* box = lv_obj_create(parent);
    lv_obj_remove_style_all(box);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(box, x, y);
    lv_obj_set_size(box, w, h);

    return box;
}

static lv_obj_t* create_label(lv_obj_t* parent, const lv_font_t* font)
{
    lv_obj_t* label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, 0);
    lv_label_set_text(label, "");

    return label;
}

static lv_obj_t* create_centered_label(lv_obj_t* parent, const lv_font_t* font)
{
    lv_obj_t* label = create_label(parent, font);
    lv_obj_set_width(label, SCREEN_WIDTH);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

    return label;
}

static lv_obj_t* create_answer(lv_obj_t* parent, int32_t x, const lv_font_t* font, const char* text)
{
    lv_obj_t* answer = create_box(parent, x, 0, 50, 28);
    lv_obj_set_style_radius(answer, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(answer, lv_color_hex(COLOR_ACCENT), 0);

    lv_obj_t* label = create_label(answer, font);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return answer;
}

static lv_obj_t* create_bar(lv_obj_t* parent)
{
    lv_obj_t* bar = lv_bar_create(parent);
    lv_obj_remove_style_all(bar);
    lv_obj_set_pos(bar, 10, 181);
    lv_obj_set_size(bar, 115, 5);
    lv_bar_set_range(bar, 0, 100);

    lv_obj_set_style_radius(bar, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(COLOR_BAR_TRACK), LV_PART_MAIN);
    lv_obj_set_style_radius(bar, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar, lv_color_white(), LV_PART_INDICATOR);

    return bar;
}

static lv_obj_t* create_dots(lv_obj_t* parent)
{
    lv_obj_t* row = create_box(parent, 50, 225, DOT_STEP * (DOT_COUNT - 1) + DOT_SIZE, DOT_SIZE);
    for (size_t i = 0; i < DOT_COUNT; i++) {
        s_dots[i] = create_box(row, DOT_STEP * i, 0, DOT_SIZE, DOT_SIZE);
        lv_obj_set_style_radius(s_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(s_dots[i], LV_OPA_COVER, 0);
    }

    return row;
}

void ui_widgets_create(lv_obj_t* screen, const ui_fonts_t* fonts)
{
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(screen, lv_color_white(), 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    // Battery percentage ends just left of the battery icon at x=74.
    s_widgets[UI_W_BATTERY_TEXT] = create_label(screen, fonts->digits_10);
    lv_obj_set_pos(s_widgets[UI_W_BATTERY_TEXT], 41, 12);
    lv_obj_set_width(s_widgets[UI_W_BATTERY_TEXT], 30);
    lv_obj_set_style_text_align(s_widgets[UI_W_BATTERY_TEXT], LV_TEXT_ALIGN_RIGHT, 0);
    s_widgets[UI_W_BATTERY] = lv_image_create(screen);
    ui_charge_init(s_widgets[UI_W_BATTERY]);

    s_widgets[UI_W_TITLE] = create_label(screen, fonts->text_30);
    s_widgets[UI_W_PLATE] = lv_image_create(screen);
    s_widgets[UI_W_VALUE] = create_centered_label(screen, fonts->digits_60);
    lv_obj_set_pos(s_widgets[UI_W_VALUE], 0, 72);

    s_widgets[UI_W_FACE] = lv_image_create(screen);
    ui_blink_init(s_widgets[UI_W_FACE]);

    s_widgets[UI_W_CAPTION] = create_centered_label(screen, fonts->text_15);
    s_widgets[UI_W_SUN] = lv_image_create(screen);
    lv_image_set_src(s_widgets[UI_W_SUN], UI_IMG("misc/sun"));
    lv_obj_set_pos(s_widgets[UI_W_SUN], 40, 60);
    s_widgets[UI_W_BAR] = create_bar(screen);

    s_widgets[UI_W_ANSWERS] = create_box(screen, 0, 200, SCREEN_WIDTH, 28);
    s_yes = create_answer(s_widgets[UI_W_ANSWERS], 15, fonts->text_15, "ДА");
    s_no = create_answer(s_widgets[UI_W_ANSWERS], 70, fonts->text_15, "НЕТ");
    s_widgets[UI_W_DOTS] = create_dots(screen);

    // 60x99, centered on the 135x240 screen.
    s_widgets[UI_W_LIGHTNING] = lv_image_create(screen);
    lv_image_set_src(s_widgets[UI_W_LIGHTNING], UI_IMG("misc/lightning_charge"));
    lv_obj_set_pos(s_widgets[UI_W_LIGHTNING], 38, 70);

    for (size_t i = 0; i < UI_W_COUNT; i++) {
        lv_obj_add_flag(s_widgets[i], LV_OBJ_FLAG_HIDDEN);
    }
}

uint32_t ui_show_battery(const char* text, const char* icon_src, int32_t icon_y)
{
    ui_charge_set(NULL, 0);
    set_text(s_widgets[UI_W_BATTERY_TEXT], text);
    if (icon_src == NULL) {
        return UI_SHOW(UI_W_BATTERY_TEXT);
    }

    set_pos(s_widgets[UI_W_BATTERY], 74, icon_y);
    set_src(s_widgets[UI_W_BATTERY], icon_src);

    return UI_SHOW(UI_W_BATTERY_TEXT) | UI_SHOW(UI_W_BATTERY);
}

// Without the percentage the 21 px icon alone is centered.
uint32_t ui_show_battery_animation(const char* const* frames, size_t count, int32_t icon_y)
{
    set_pos(s_widgets[UI_W_BATTERY], 57, icon_y);
    ui_charge_set(frames, count);

    return UI_SHOW(UI_W_BATTERY);
}

uint32_t ui_show_lightning(void)
{
    return UI_SHOW(UI_W_LIGHTNING);
}

uint32_t ui_show_title(const char* text, int32_t x, int32_t y)
{
    set_pos(s_widgets[UI_W_TITLE], x, y);
    set_text(s_widgets[UI_W_TITLE], text);

    return UI_SHOW(UI_W_TITLE);
}

uint32_t ui_show_plate(const char* src, int32_t x, int32_t y)
{
    set_pos(s_widgets[UI_W_PLATE], x, y);
    set_src(s_widgets[UI_W_PLATE], src);

    return UI_SHOW(UI_W_PLATE);
}

uint32_t ui_show_value(const char* text, const lv_font_t* font)
{
    set_font(s_widgets[UI_W_VALUE], font);
    set_text(s_widgets[UI_W_VALUE], text);

    return UI_SHOW(UI_W_VALUE);
}

uint32_t ui_show_caption(const char* text, int32_t y)
{
    set_pos(s_widgets[UI_W_CAPTION], 0, y);
    set_text(s_widgets[UI_W_CAPTION], text);

    return UI_SHOW(UI_W_CAPTION);
}

uint32_t ui_show_bar(uint8_t percent)
{
    lv_obj_t* bar = s_widgets[UI_W_BAR];
    if (lv_bar_get_value(bar) != percent) {
        lv_bar_set_value(bar, percent, LV_ANIM_OFF);
    }

    return UI_SHOW(UI_W_BAR);
}

uint32_t ui_show_answers(bool yes_selected)
{
    const lv_color_t accent = lv_color_hex(COLOR_ACCENT);
    set_bg(s_yes, accent, yes_selected ? LV_OPA_COVER : LV_OPA_TRANSP);
    set_bg(s_no, accent, yes_selected ? LV_OPA_TRANSP : LV_OPA_COVER);

    return UI_SHOW(UI_W_ANSWERS);
}

uint32_t ui_show_dots(size_t active)
{
    for (size_t i = 0; i < DOT_COUNT; i++) {
        const lv_color_t color = i == active ? lv_color_white() : lv_color_hex(COLOR_INACTIVE_DOT);
        set_bg(s_dots[i], color, LV_OPA_COVER);
    }

    return UI_SHOW(UI_W_DOTS);
}

uint32_t ui_show_face_image(const char* open_src, const char* closed_src, int32_t x, int32_t y)
{
    set_pos(s_widgets[UI_W_FACE], x, y);
    ui_blink_set(open_src, closed_src);

    return UI_SHOW(UI_W_FACE);
}

void ui_widgets_set_visible(uint32_t visible)
{
    // Hidden images must not keep reading frames from flash.
    if ((visible & UI_SHOW(UI_W_FACE)) == 0) {
        ui_blink_set(NULL, NULL);
    }

    if ((visible & UI_SHOW(UI_W_BATTERY)) == 0) {
        ui_charge_set(NULL, 0);
    }

    for (size_t i = 0; i < UI_W_COUNT; i++) {
        set_visible(s_widgets[i], (visible & UI_SHOW(i)) != 0);
    }
}
