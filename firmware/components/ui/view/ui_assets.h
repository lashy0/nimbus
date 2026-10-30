#pragma once

/**
 * @file
 * Asset paths. "S:" is LVGL's stdio drive, mapped to the assets partition.
 */

/** Image from assets/img, e.g. UI_IMAGE("face/happy"). */
#define UI_IMAGE(name) "S:img/" name ".bin"

/** Font from assets/fonts, e.g. UI_FONT("semibold_15"). */
#define UI_FONT(name) "S:fonts/" name ".fnt"
