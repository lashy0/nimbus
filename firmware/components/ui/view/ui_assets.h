#pragma once

#ifndef UI_ASSET_ROOT
#    define UI_ASSET_ROOT "S:/spiffs"
#endif

// Shared image paths live for the entire boot and are borrowed by widgets.
extern const char UI_IMAGE_BATTERY_LOW[];
extern const char UI_IMAGE_BATTERY_MID[];
extern const char UI_IMAGE_BATTERY_HIGH[];
extern const char UI_IMAGE_BATTERY_FULL[];
extern const char UI_IMAGE_BATTERY_CHARGING_LOW[];
extern const char UI_IMAGE_BATTERY_CHARGING_MID[];
extern const char UI_IMAGE_BATTERY_CHARGING_HIGH[];
extern const char UI_IMAGE_BATTERY_CHARGING_FULL[];
extern const char UI_IMAGE_BASE[];
extern const char UI_IMAGE_SUN[];
extern const char UI_IMAGE_LIGHTNING[];
extern const char UI_IMAGE_FACE_ULTRA_HAPPY[];
extern const char UI_IMAGE_FACE_HAPPY[];
extern const char UI_IMAGE_FACE_HAPPY_CLOSED_EYES[];
extern const char UI_IMAGE_FACE_ORDINARY[];
extern const char UI_IMAGE_FACE_ORDINARY_CLOSED_EYES[];
extern const char UI_IMAGE_FACE_ORDINARY_NIMBUS[];
extern const char UI_IMAGE_FACE_DIZZY_NIMBUS[];
extern const char UI_IMAGE_FACE_SAD[];
extern const char UI_IMAGE_FACE_DIZZY[];
extern const char UI_IMAGE_FACE_DEAD[];
extern const char UI_IMAGE_FACE_DIVER[];
extern const char UI_IMAGE_FACE_DIVER_CLOSED_EYES[];
extern const char UI_IMAGE_FACE_CAT_HUH[];
extern const char UI_IMAGE_FACE_TEMP_MINUS[];
extern const char UI_IMAGE_FACE_TEMP_MINUS_CLOSED_EYES[];
extern const char UI_IMAGE_FACE_TEMP_NORMAL[];
extern const char UI_IMAGE_FACE_TEMP_NORMAL_CLOSED_EYES[];
extern const char UI_IMAGE_FACE_TEMP_PLUS[];
extern const char UI_IMAGE_STATUS_GOOD[];
extern const char UI_IMAGE_STATUS_MID[];
extern const char UI_IMAGE_STATUS_BAD[];
extern const char UI_IMAGE_STATUS_WARN[];
extern const char UI_IMAGE_STATUS_CRIT[];
extern const char UI_IMAGE_STATUS_MINUS[];
extern const char UI_IMAGE_STATUS_NOTHING[];
extern const char UI_IMAGE_STATUS_PLUS[];
extern const char UI_IMAGE_STATUS_DRY[];
extern const char UI_IMAGE_STATUS_DAMP[];

enum { UI_BATTERY_CHARGING_COUNT = 4 };

extern const char* const UI_BATTERY_CHARGING[UI_BATTERY_CHARGING_COUNT];
