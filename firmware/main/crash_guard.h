#pragma once

#include <stdbool.h>

/**
 * @file
 * Stops a crash loop. Panics, watchdog resets and brownouts in a row are
 * counted across resets; a power-on starts the count over.
 */

/**
 * Counts this boot and logs what the last crash left in flash.
 *
 * Call once, first at boot.
 *
 * @return true if this is the 3rd crash in a row and the boot must stop. After
 *         that, each further boot gets one try.
 */
bool crash_guard_check(void);

/** Starts the count over once the firmware has run 5 min; call periodically. */
void crash_guard_poll(void);
