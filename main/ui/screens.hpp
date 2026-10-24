#pragma once
#include <lvgl.h>

enum class ScreenID {
    BOOT,
    HOME,
    WIND,
    FOCUS,          // includes the PEEK and DRIFT states
    INTERMISSION,
    MENU,
    WORKSHOP,
    CIRCLE,
    TIMER_SETTINGS,
    COUNT
};

const char *screen_name(ScreenID id);

// Every screen builder gets the content area to build into and a fresh lv_group for the
// encoder. Add focusable widgets to the group in knob order.
typedef void (*ScreenBuilder)(lv_obj_t *parent, lv_group_t *group);

// Knob navigation test: a menu of every screen. Stands in for screens not built yet.
void screen_placeholder(lv_obj_t *parent, lv_group_t *group);

// Touch calibration check (POMO_TOUCH_TEST)
void screen_touch_test(lv_obj_t *parent);
