#include "ui_faces.h"

#include "ui_assets.h"
#include "ui_widgets.h"

typedef struct {
    const char* open;
    const char* closed; // NULL: the artwork has no closed-eyes variant.
    int32_t x;
    int32_t y;
} face_t;

static const face_t FACES[] = {
    [UI_FACE_BASE] = {UI_IMAGE_BASE, NULL, 32, 83},
    [UI_FACE_ULTRA_HAPPY] = {UI_IMAGE_FACE_ULTRA_HAPPY, NULL, 32, 137},
    [UI_FACE_HAPPY] = {UI_IMAGE_FACE_HAPPY, UI_IMAGE_FACE_HAPPY_CLOSED_EYES, 32, 137},
    [UI_FACE_ORDINARY] = {UI_IMAGE_FACE_ORDINARY, UI_IMAGE_FACE_ORDINARY_CLOSED_EYES, 32, 137},
    [UI_FACE_NIMBUS] = {UI_IMAGE_FACE_ORDINARY_NIMBUS, NULL, 25, 83},
    [UI_FACE_DIZZY_NIMBUS] = {UI_IMAGE_FACE_DIZZY_NIMBUS, NULL, 26, 45},
    [UI_FACE_SAD] = {UI_IMAGE_FACE_SAD, NULL, 32, 137},
    [UI_FACE_DIZZY] = {UI_IMAGE_FACE_DIZZY, NULL, 32, 137},
    [UI_FACE_DEAD] = {UI_IMAGE_FACE_DEAD, NULL, 32, 137},
    [UI_FACE_DIVER] = {UI_IMAGE_FACE_DIVER, UI_IMAGE_FACE_DIVER_CLOSED_EYES, 32, 137},
    [UI_FACE_HUH] = {UI_IMAGE_FACE_CAT_HUH, NULL, 32, 45},
    [UI_FACE_TEMP_MINUS] = {UI_IMAGE_FACE_TEMP_MINUS, UI_IMAGE_FACE_TEMP_MINUS_CLOSED_EYES, 31, 127},
    [UI_FACE_TEMP_NORMAL] = {UI_IMAGE_FACE_TEMP_NORMAL, UI_IMAGE_FACE_TEMP_NORMAL_CLOSED_EYES, 31, 127},
    [UI_FACE_TEMP_PLUS] = {UI_IMAGE_FACE_TEMP_PLUS, NULL, 31, 127},
};

uint32_t ui_show_face(ui_face_t face)
{
    const face_t* entry = &FACES[face];

    return ui_show_face_image(entry->open, entry->closed, entry->x, entry->y);
}
