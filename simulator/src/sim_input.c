#include "sim_input.h"

#include <SDL.h>

#include "lvgl.h"

#include "sim_data.h"
#include "sim_log.h"

#define LONG_PRESS_MS 1500
#define HOLD_CHECK_MS 20
#define QUEUE_LENGTH 8
#define KEY_COUNT 2

typedef struct {
    bool held;
    bool long_sent;
    uint32_t pressed_ms;
} key_state_t;

static key_state_t s_keys[KEY_COUNT];
static buttons_event_t s_queue[QUEUE_LENGTH];
static size_t s_head;
static size_t s_count;

// Presses that do not fit the queue are dropped, as on the device.
static void send(buttons_id_t id, buttons_press_t press)
{
    if (s_count < QUEUE_LENGTH) {
        s_queue[(s_head + s_count) % QUEUE_LENGTH] = (buttons_event_t){.id = id, .press = press};
        s_count++;
    }

    sim_log("button", "%s %s", id == BUTTONS_PREV ? "PREV" : "NEXT", press == BUTTONS_PRESS_LONG ? "long" : "short");
}

static bool button_of(SDL_Scancode key, buttons_id_t* id)
{
    if (key != SDL_SCANCODE_LEFT && key != SDL_SCANCODE_RIGHT) {
        return false;
    }

    *id = key == SDL_SCANCODE_LEFT ? BUTTONS_PREV : BUTTONS_NEXT;

    return true;
}

static int on_event(void* context, SDL_Event* event)
{
    if ((event->type != SDL_KEYDOWN && event->type != SDL_KEYUP) || event->key.repeat) {
        return 1;
    }

    // Scancodes keep the keys in place under any keyboard layout.
    const SDL_Scancode key = event->key.keysym.scancode;
    buttons_id_t id;
    if (!button_of(key, &id)) {
        if (event->type == SDL_KEYDOWN && key >= SDL_SCANCODE_A && key <= SDL_SCANCODE_Z) {
            sim_data_key((char)('a' + (key - SDL_SCANCODE_A)));
        }

        return 1;
    }

    key_state_t* state = &s_keys[id];
    if (event->type == SDL_KEYDOWN) {
        *state = (key_state_t){.held = true, .pressed_ms = lv_tick_get()};
    } else if (state->held) {
        if (!state->long_sent) {
            send(id, BUTTONS_PRESS_SHORT);
        }
        state->held = false;
    }

    return 1;
}

static void on_hold_check(lv_timer_t* timer)
{
    for (size_t id = 0; id < KEY_COUNT; id++) {
        key_state_t* state = &s_keys[id];
        if (state->held && !state->long_sent && lv_tick_elaps(state->pressed_ms) >= LONG_PRESS_MS) {
            state->long_sent = true;
            send((buttons_id_t)id, BUTTONS_PRESS_LONG);
        }
    }
}

void sim_input_start(void)
{
    SDL_AddEventWatch(on_event, NULL);
    lv_timer_create(on_hold_check, HOLD_CHECK_MS, NULL);
}

bool sim_input_receive(buttons_event_t* out)
{
    if (s_count == 0) {
        return false;
    }

    *out = s_queue[s_head];
    s_head = (s_head + 1) % QUEUE_LENGTH;
    s_count--;

    return true;
}
