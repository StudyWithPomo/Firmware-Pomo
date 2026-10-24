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

void screen_boot(lv_obj_t *parent, lv_group_t *group);
void screen_home(lv_obj_t *parent, lv_group_t *group);
void screen_wind(lv_obj_t *parent, lv_group_t *group);
void screen_focus(lv_obj_t *parent, lv_group_t *group);
void screen_intermission(lv_obj_t *parent, lv_group_t *group);
void screen_menu(lv_obj_t *parent, lv_group_t *group);
void screen_workshop(lv_obj_t *parent, lv_group_t *group);
void screen_circle(lv_obj_t *parent, lv_group_t *group);
void screen_timer_settings(lv_obj_t *parent, lv_group_t *group);

// Touch calibration check (POMO_TOUCH_TEST)
void screen_touch_test(lv_obj_t *parent);
