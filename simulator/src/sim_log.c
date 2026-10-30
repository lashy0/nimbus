#include "sim_log.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <io.h>
#include <windows.h>

#include "lvgl.h"

typedef struct {
    const char* name;
    const char* color; // ANSI escape sequence.
} level_t;

static const level_t LEVEL_INFO = {"INFO", "\x1b[32m"};
static const level_t LEVEL_WARN = {"WARN", "\x1b[33m"};
static const level_t LEVEL_ERROR = {"ERROR", "\x1b[31m"};

#define COLOR_RESET "\x1b[0m"

static bool s_colors;

static void write_line(const level_t* level, const char* source, const char* message)
{
    const uint32_t ms = lv_tick_get();
    printf(
        "%4u.%03u  %s%-5s%s  %-7s %s\n",
        (unsigned)(ms / 1000),
        (unsigned)(ms % 1000),
        s_colors ? level->color : "",
        level->name,
        s_colors ? COLOR_RESET : "",
        source,
        message);
}

// LVGL passes "[Level] function: message" and a line break.
static void on_lvgl_log(lv_log_level_t level, const char* text)
{
    const char* message = strchr(text, ']');
    message = message != NULL ? message + 2 : text;

    char line[256];
    snprintf(line, sizeof(line), "%s", message);
    line[strcspn(line, "\r\n")] = '\0';

    const level_t* shown = level >= LV_LOG_LEVEL_ERROR  ? &LEVEL_ERROR
                           : level == LV_LOG_LEVEL_WARN ? &LEVEL_WARN
                                                        : &LEVEL_INFO;
    write_line(shown, "lvgl", line);
}

void sim_log_start(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    SetConsoleOutputCP(CP_UTF8);

    // Colors only in a console that understands them.
    const HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    s_colors = _isatty(_fileno(stdout)) && GetConsoleMode(console, &mode) &&
               SetConsoleMode(console, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    lv_log_register_print_cb(on_lvgl_log);
}

void sim_log(const char* source, const char* format, ...)
{
    char message[256];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    write_line(&LEVEL_INFO, source, message);
}
