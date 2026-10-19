#include "ui_widgets.h"

typedef struct {
    const char* open;
    const char* closed; // NULL: the artwork has no closed-eyes variant.
    lv_coord_t x;
    lv_coord_t y;
} face_t;

static const face_t FACES[] = {
    [UI_FACE_BASE] = {UI_IMG("misc/base"), NULL, 32, 83},
    [UI_FACE_ULTRA_HAPPY] = {UI_IMG("face/ultra_happy"), NULL, 32, 137},
    [UI_FACE_HAPPY] = {UI_IMG("face/happy"), UI_IMG("face/happy_closed_eyes"), 32, 137},
    [UI_FACE_ORDINARY] = {UI_IMG("face/ordinary"), UI_IMG("face/ordinary_closed_eyes"), 32, 137},
    [UI_FACE_NIMBUS] = {UI_IMG("face/ordinary_nimbus"), NULL, 25, 83},
    [UI_FACE_SAD] = {UI_IMG("face/sad"), NULL, 32, 137},
    [UI_FACE_DIZZY] = {UI_IMG("face/dizzy"), NULL, 32, 137},
    [UI_FACE_DEAD] = {UI_IMG("face/dead"), NULL, 32, 137},
    [UI_FACE_DIVER] = {UI_IMG("face/diver"), UI_IMG("face/diver_closed_eyes"), 32, 137},
    [UI_FACE_HUH] = {UI_IMG("face/cat_huh"), NULL, 32, 45},
    [UI_FACE_TEMP_MINUS] = {UI_IMG("face/temp_minus"), UI_IMG("face/temp_minus_closed_eyes"), 31, 127},
    [UI_FACE_TEMP_NORMAL] = {UI_IMG("face/temp_normal"), UI_IMG("face/temp_normal_closed_eyes"), 31, 127},
    [UI_FACE_TEMP_PLUS] = {UI_IMG("face/temp_plus"), NULL, 31, 127},
};

uint32_t ui_show_face(ui_face_t face)
{
    const face_t* entry = &FACES[face];

    return ui_show_face_image(entry->open, entry->closed, entry->x, entry->y);
}
