#include "ui_view.h"

#include "ui_page.h"

typedef struct {
    lv_obj_t* (*create)(void);
    void (*update)(const sensing_snapshot_t* snapshot);
} page_t;

// Measurement pages in the order NEXT walks them.
static const page_t PAGES[] = {
    {ui_page_iaq_create, ui_page_iaq_update},
    {ui_page_temp_create, ui_page_temp_update},
    {ui_page_hum_create, ui_page_hum_update},
};

#define PAGE_COUNT (sizeof(PAGES) / sizeof(PAGES[0]))

static lv_obj_t* s_start;
static lv_obj_t* s_error;
static lv_obj_t* s_pages[PAGE_COUNT];
static size_t s_selected;
static sensing_snapshot_t s_snapshot; // NO_DATA until the first update.

void ui_view_create(void)
{
    s_start = ui_page_start_create();
    s_error = ui_page_error_create();
    for (size_t i = 0; i < PAGE_COUNT; i++) {
        s_pages[i] = PAGES[i].create();
    }

    lv_obj_t* initial = lv_screen_active();
    lv_screen_load(s_start);
    lv_obj_delete(initial);
}

static void show(lv_obj_t* page)
{
    if (lv_screen_active() != page) {
        lv_screen_load(page);
    }
}

static void refresh(void)
{
    switch (s_snapshot.status) {
        case SENSING_STATUS_NO_DATA:
            show(s_start);
            break;
        case SENSING_STATUS_OK:
            PAGES[s_selected].update(&s_snapshot);
            show(s_pages[s_selected]);
            break;
        case SENSING_STATUS_ERROR:
            show(s_error);
            break;
    }
}

void ui_view_update(const sensing_snapshot_t* snapshot)
{
    s_snapshot = *snapshot;
    refresh();
}

void ui_view_press(const buttons_event_t* event)
{
    // Long presses have no action yet.
    if (event->press != BUTTONS_PRESS_SHORT || s_snapshot.status != SENSING_STATUS_OK) {
        return;
    }

    const size_t step = event->id == BUTTONS_NEXT ? 1 : PAGE_COUNT - 1;
    s_selected = (s_selected + step) % PAGE_COUNT;
    refresh();
}
