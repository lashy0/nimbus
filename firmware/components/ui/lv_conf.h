#ifndef LV_CONF_H
#define LV_CONF_H

// A failed LVGL assertion must enter panic while its original stack is intact;
// the default endless loop only surfaces later as a watchdog timeout.
#define LV_ASSERT_HANDLER_INCLUDE <stdlib.h>
#define LV_ASSERT_HANDLER abort();

#endif
