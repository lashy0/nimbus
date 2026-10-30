#include "ui_view.h"

#include "ui_page.h"

static lv_obj_t* s_start;
static lv_obj_t* s_iaq;
static lv_obj_t* s_error;

void ui_view_create(void)
{
    s_start = ui_page_start_create();
    s_iaq = ui_page_iaq_create();
    s_error = ui_page_error_create();

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

void ui_view_update(const sensing_snapshot_t* snapshot)
{
    switch (snapshot->status) {
        case SENSING_STATUS_NO_DATA:
            show(s_start);
            break;
        case SENSING_STATUS_OK:
            ui_page_iaq_update(snapshot);
            show(s_iaq);
            break;
        case SENSING_STATUS_ERROR:
            show(s_error);
            break;
    }
}
