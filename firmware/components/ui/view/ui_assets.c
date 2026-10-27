#include "ui_assets.h"

#define IMAGE(path) UI_ASSET_ROOT "/img/" path ".bin"

const char UI_IMAGE_BATTERY_LOW[] = IMAGE("batt/1_not_charging");
const char UI_IMAGE_BATTERY_MID[] = IMAGE("batt/2_not_charging");
const char UI_IMAGE_BATTERY_HIGH[] = IMAGE("batt/3_not_charging");
const char UI_IMAGE_BATTERY_FULL[] = IMAGE("batt/full_not_charging");
const char UI_IMAGE_BATTERY_CHARGING_LOW[] = IMAGE("batt/1_charging");
const char UI_IMAGE_BATTERY_CHARGING_MID[] = IMAGE("batt/2_charging");
const char UI_IMAGE_BATTERY_CHARGING_HIGH[] = IMAGE("batt/3_charging");
const char UI_IMAGE_BATTERY_CHARGING_FULL[] = IMAGE("batt/full_charging");
const char UI_IMAGE_BASE[] = IMAGE("misc/base");
const char UI_IMAGE_SUN[] = IMAGE("misc/sun");
const char UI_IMAGE_LIGHTNING[] = IMAGE("misc/lightning_charge");
const char UI_IMAGE_FACE_ULTRA_HAPPY[] = IMAGE("face/ultra_happy");
const char UI_IMAGE_FACE_HAPPY[] = IMAGE("face/happy");
const char UI_IMAGE_FACE_HAPPY_CLOSED_EYES[] = IMAGE("face/happy_closed_eyes");
const char UI_IMAGE_FACE_ORDINARY[] = IMAGE("face/ordinary");
const char UI_IMAGE_FACE_ORDINARY_CLOSED_EYES[] = IMAGE("face/ordinary_closed_eyes");
const char UI_IMAGE_FACE_ORDINARY_NIMBUS[] = IMAGE("face/ordinary_nimbus");
const char UI_IMAGE_FACE_DIZZY_NIMBUS[] = IMAGE("face/dizzy_nimbus");
const char UI_IMAGE_FACE_SAD[] = IMAGE("face/sad");
const char UI_IMAGE_FACE_DIZZY[] = IMAGE("face/dizzy");
const char UI_IMAGE_FACE_DEAD[] = IMAGE("face/dead");
const char UI_IMAGE_FACE_DIVER[] = IMAGE("face/diver");
const char UI_IMAGE_FACE_DIVER_CLOSED_EYES[] = IMAGE("face/diver_closed_eyes");
const char UI_IMAGE_FACE_CAT_HUH[] = IMAGE("face/cat_huh");
const char UI_IMAGE_FACE_TEMP_MINUS[] = IMAGE("face/temp_minus");
const char UI_IMAGE_FACE_TEMP_MINUS_CLOSED_EYES[] = IMAGE("face/temp_minus_closed_eyes");
const char UI_IMAGE_FACE_TEMP_NORMAL[] = IMAGE("face/temp_normal");
const char UI_IMAGE_FACE_TEMP_NORMAL_CLOSED_EYES[] = IMAGE("face/temp_normal_closed_eyes");
const char UI_IMAGE_FACE_TEMP_PLUS[] = IMAGE("face/temp_plus");
const char UI_IMAGE_STATUS_GOOD[] = IMAGE("status/good");
const char UI_IMAGE_STATUS_MID[] = IMAGE("status/mid");
const char UI_IMAGE_STATUS_BAD[] = IMAGE("status/bad");
const char UI_IMAGE_STATUS_WARN[] = IMAGE("status/warn");
const char UI_IMAGE_STATUS_CRIT[] = IMAGE("status/crit");
const char UI_IMAGE_STATUS_MINUS[] = IMAGE("status/minus");
const char UI_IMAGE_STATUS_NOTHING[] = IMAGE("status/nothing");
const char UI_IMAGE_STATUS_PLUS[] = IMAGE("status/plus");
const char UI_IMAGE_STATUS_DRY[] = IMAGE("status/dry");
const char UI_IMAGE_STATUS_DAMP[] = IMAGE("status/damp");

const char* const UI_BATTERY_CHARGING[UI_BATTERY_CHARGING_COUNT] = {
    UI_IMAGE_BATTERY_CHARGING_LOW,
    UI_IMAGE_BATTERY_CHARGING_MID,
    UI_IMAGE_BATTERY_CHARGING_HIGH,
    UI_IMAGE_BATTERY_CHARGING_FULL,
};
