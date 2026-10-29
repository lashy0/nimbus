/**
 * @file
 * LVGL settings that Kconfig cannot set. The rest is in sdkconfig.defaults.
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_ASSERT_HANDLER_INCLUDE <stdlib.h>
#define LV_ASSERT_HANDLER abort(); /**< Panic while the failing stack is intact. */

#endif
