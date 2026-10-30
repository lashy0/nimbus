/**
 * @file
 * LVGL settings of the simulator. They repeat the LVGL options of
 * firmware/sdkconfig.defaults and firmware/components/ui/lv_conf.h; the rest
 * are LVGL defaults, as in the firmware.
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16

#define LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB

#define LV_USE_LOG 1
#define LV_LOG_USE_TIMESTAMP 0 // sim_log adds its own.
#define LV_LOG_USE_FILE_LINE 0

#define LV_ASSERT_HANDLER_INCLUDE <stdlib.h>
#define LV_ASSERT_HANDLER abort();

// Images and fonts straight from firmware/assets.
#define LV_USE_FS_STDIO 1
#define LV_FS_STDIO_LETTER 'S'
#define LV_FS_STDIO_PATH NIMBUS_ASSETS_DIR

#define LV_USE_SDL 1
#define LV_SDL_INCLUDE_PATH <SDL.h>

#endif
