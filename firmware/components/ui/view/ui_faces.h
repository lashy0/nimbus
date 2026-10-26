#pragma once

#include <stdint.h>

typedef enum {
    UI_FACE_BASE,
    UI_FACE_ULTRA_HAPPY,
    UI_FACE_HAPPY,
    UI_FACE_ORDINARY,
    UI_FACE_NIMBUS,
    UI_FACE_DIZZY_NIMBUS,
    UI_FACE_SAD,
    UI_FACE_DIZZY,
    UI_FACE_DEAD,
    UI_FACE_DIVER,
    UI_FACE_HUH,
    UI_FACE_TEMP_MINUS,
    UI_FACE_TEMP_NORMAL,
    UI_FACE_TEMP_PLUS,
} ui_face_t;

// Sets the face image and placement and returns the widget visibility bit.
// Call under the renderer lock.
uint32_t ui_show_face(ui_face_t face);
