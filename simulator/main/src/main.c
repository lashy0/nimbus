#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef NIMBUS_USE_FREERTOS
#    include "FreeRTOS.h"
#    include "task.h"
#endif

#include "SDL.h"
#include "lvgl/lvgl.h"

#include "preview.h"
#include "ui_fonts.h"
#include "ui_view.h"

#define INPUT_CAPACITY 32

typedef struct {
    SDL_Event event;
    int64_t timestamp_ms;
} input_event_t;

typedef struct {
    SDL_mutex* mutex;
    Uint32 window_id;
    input_event_t events[INPUT_CAPACITY];
    size_t count;
    bool lost;
    bool watching;
} input_t;

static ui_fonts_t s_fonts;
static preview_t s_preview;
static input_t s_input;
static lv_display_t* s_display;
static lv_obj_t* s_dimmer;

static bool controlled_key(SDL_Scancode key)
{
    return key == SDL_SCANCODE_LEFT || key == SDL_SCANCODE_RIGHT || key == SDL_SCANCODE_R || key == SDL_SCANCODE_U ||
           key == SDL_SCANCODE_E || key == SDL_SCANCODE_C || key == SDL_SCANCODE_UP || key == SDL_SCANCODE_DOWN ||
           key == SDL_SCANCODE_PAGEUP || key == SDL_SCANCODE_PAGEDOWN || key == SDL_SCANCODE_HOME;
}

static int watch_input(void* context, SDL_Event* event)
{
    input_t* input = context;
    const bool key_event = event->type == SDL_KEYDOWN || event->type == SDL_KEYUP;
    const bool focus_lost = event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_FOCUS_LOST;
    if (key_event) {
        if (event->key.windowID != input->window_id || event->key.repeat ||
            !controlled_key(event->key.keysym.scancode)) {
            return 1;
        }
    } else if (!focus_lost || event->window.windowID != input->window_id) {
        return 1;
    }

    // SDL may invoke event watches on another thread; only the UI loop touches app/LVGL.
    SDL_LockMutex(input->mutex);
    if (input->count == INPUT_CAPACITY) {
        input->lost = true;
    } else if (!input->lost) {
        input->events[input->count++] = (input_event_t){.event = *event, .timestamp_ms = (int64_t)SDL_GetTicks64()};
    }

    SDL_UnlockMutex(input->mutex);

    return 1;
}

static void present(uint32_t effects)
{
    if (effects == APP_EFFECT_NONE) {
        return;
    }

    if (effects & APP_EFFECT_MODEL) {
        ui_model_t model;
        app_state_model(&s_preview.app, &model);
        ui_view_apply(&model);
    }

    const bool shutdown = s_preview.app.mode == APP_MODE_SHUTTING_DOWN;
    const uint8_t brightness = shutdown ? 0 : app_state_backlight(&s_preview.app);
    // Approximate gamma correction keeps simulated midrange backlight levels from looking too dark.
    const float intensity = powf(brightness / 100.0f, 1.0f / 2.2f);
    lv_obj_set_style_bg_opa(s_dimmer, (lv_opa_t)lroundf((1.0f - intensity) * LV_OPA_COVER), 0);
    if (effects & APP_EFFECT_DISPLAY) {
        app_state_display_result(&s_preview.app, true, (int64_t)SDL_GetTicks64());
    }
}

static void process_input(void)
{
    input_event_t events[INPUT_CAPACITY];
    SDL_LockMutex(s_input.mutex);
    const size_t count = s_input.lost ? 0 : s_input.count;
    const bool lost = s_input.lost;
    memcpy(events, s_input.events, count * sizeof(events[0]));
    s_input.count = 0;
    s_input.lost = false;
    SDL_UnlockMutex(s_input.mutex);

    if (lost) {
        present(preview_input_lost(&s_preview));
    }

    for (size_t i = 0; i < count; i++) {
        const SDL_Event* event = &events[i].event;
        const int64_t now_ms = events[i].timestamp_ms;
        uint32_t effects = APP_EFFECT_NONE;
        if (event->type == SDL_WINDOWEVENT) {
            effects = preview_input_lost(&s_preview);
        } else {
            const SDL_Scancode key = event->key.keysym.scancode;
            const bool pressed = event->type == SDL_KEYDOWN;
            if (key == SDL_SCANCODE_LEFT || key == SDL_SCANCODE_RIGHT) {
                effects = preview_button(
                    &s_preview, key == SDL_SCANCODE_LEFT ? APP_BUTTON_PREV : APP_BUTTON_NEXT, pressed, now_ms);
            } else if (pressed) {
                switch (key) {
                    case SDL_SCANCODE_R:
                        effects = preview_restart(&s_preview, now_ms);
                        break;
                    case SDL_SCANCODE_U:
                        effects = preview_toggle_usb(&s_preview, now_ms);
                        break;
                    case SDL_SCANCODE_E:
                        effects = preview_toggle_error(&s_preview, now_ms);
                        break;
                    case SDL_SCANCODE_C:
                        effects = preview_toggle_calibration(&s_preview, now_ms);
                        break;
                    case SDL_SCANCODE_UP:
                    case SDL_SCANCODE_DOWN: {
                        const bool fast = (event->key.keysym.mod & KMOD_SHIFT) != 0;
                        const preview_value_edit_t edit =
                            key == SDL_SCANCODE_UP ? (fast ? PREVIEW_VALUE_INCREASE_FAST : PREVIEW_VALUE_INCREASE)
                                                   : (fast ? PREVIEW_VALUE_DECREASE_FAST : PREVIEW_VALUE_DECREASE);
                        effects = preview_edit_value(&s_preview, edit, now_ms);
                        break;
                    }
                    case SDL_SCANCODE_PAGEUP:
                        effects = preview_edit_value(&s_preview, PREVIEW_VALUE_NEXT_STATE, now_ms);
                        break;
                    case SDL_SCANCODE_PAGEDOWN:
                        effects = preview_edit_value(&s_preview, PREVIEW_VALUE_PREVIOUS_STATE, now_ms);
                        break;
                    case SDL_SCANCODE_HOME:
                        effects = preview_edit_value(&s_preview, PREVIEW_VALUE_RESET, now_ms);
                        break;
                    default:
                        break;
                }
            }
        }

        present(effects);
    }
}

static void display_delete(lv_event_t* event)
{
    LV_UNUSED(event);
    if (s_input.watching) {
        SDL_DelEventWatch(watch_input, &s_input);
        s_input.watching = false;
    }

    ui_view_deinit();
    s_display = NULL;
}

int lvgl_main(void)
{
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    lv_init();
    int result = EXIT_FAILURE;

    s_display = lv_sdl_window_create(135, 240);
    if (s_display == NULL) {
        goto cleanup;
    }

    lv_sdl_window_set_title(s_display, "Nimbus Simulator");
    lv_display_set_default(s_display);
    lv_display_add_event_cb(s_display, display_delete, LV_EVENT_DELETE, NULL);
    lv_sdl_window_set_zoom(s_display, 2);
    ui_fonts_load(&s_fonts);
    if (ui_view_create(lv_display_get_screen_active(s_display), &s_fonts) != LV_RESULT_OK) {
        goto cleanup;
    }

    s_dimmer = lv_obj_create(lv_display_get_layer_top(s_display));
    lv_obj_remove_style_all(s_dimmer);
    lv_obj_set_size(s_dimmer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_dimmer, lv_color_black(), 0);
    lv_obj_remove_flag(s_dimmer, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    s_input.mutex = SDL_CreateMutex();
    if (s_input.mutex == NULL) {
        fprintf(stderr, "Could not create input mutex: %s\n", SDL_GetError());

        goto cleanup;
    }

    s_input.window_id = SDL_GetWindowID(lv_sdl_window_get_window(s_display));
    SDL_AddEventWatch(watch_input, &s_input);
    s_input.watching = true;
    present(preview_init(&s_preview, (int64_t)SDL_GetTicks64()));

    while (s_display != NULL) {
        lv_timer_handler();
        if (s_display == NULL) {
            break;
        }

        process_input();
        present(preview_tick(&s_preview, (int64_t)SDL_GetTicks64()));
#if LV_USE_OS == LV_OS_FREERTOS
        vTaskDelay(pdMS_TO_TICKS(5));
#else
        SDL_Delay(5);
#endif
    }

    result = EXIT_SUCCESS;

cleanup:
    if (s_display != NULL) {
        lv_display_delete(s_display);
    }

    if (s_input.mutex != NULL) {
        SDL_DestroyMutex(s_input.mutex);
        s_input.mutex = NULL;
    }

    ui_fonts_unload(&s_fonts);
    lv_deinit();

    return result;
}

#if LV_USE_OS == LV_OS_FREERTOS
int freertos_main(void);
#endif

int main(void)
{
#if LV_USE_OS == LV_OS_FREERTOS
    return freertos_main();
#else
    return lvgl_main();
#endif
}
