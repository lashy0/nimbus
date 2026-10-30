#pragma once

/**
 * @file
 * Log of the simulator on stdout: seconds since start, level, source and
 * message. LVGL's own warnings and errors go here too.
 */

/** Call once, after lv_init(). */
void sim_log_start(void);

/** Writes one line; source is a short word such as "button". */
void sim_log(const char* source, const char* format, ...);
