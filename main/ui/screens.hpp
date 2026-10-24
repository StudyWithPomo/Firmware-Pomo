#pragma once
#include <lvgl.h>

enum class ScreenID {
    BOOT,
    HOME,
    STUDY,          // lengths, "Always show timer", the session log, Start
    FOCUS,          // includes the PEEK and DRIFT states
    INTERMISSION,
    CIRCLE,
    PET,            // hub: Status, Playground, Wardrobe
    STATUS,
    WARDROBE,
    PLAYGROUND,
    COUNT
};

const char *screen_name(ScreenID id);

// Every screen builder gets the content area to build into and a fresh lv_group for the
// encoder. Add focusable widgets to the group in knob order.
typedef void (*ScreenBuilder)(lv_obj_t *parent, lv_group_t *group);

void screen_boot(lv_obj_t *parent, lv_group_t *group);
void screen_home(lv_obj_t *parent, lv_group_t *group);
void screen_study(lv_obj_t *parent, lv_group_t *group);
void screen_focus(lv_obj_t *parent, lv_group_t *group);
void screen_intermission(lv_obj_t *parent, lv_group_t *group);
void screen_circle(lv_obj_t *parent, lv_group_t *group);
void screen_pet(lv_obj_t *parent, lv_group_t *group);
void screen_status(lv_obj_t *parent, lv_group_t *group);
void screen_wardrobe(lv_obj_t *parent, lv_group_t *group);
void screen_playground(lv_obj_t *parent, lv_group_t *group);

// Touch calibration check (POMO_TOUCH_TEST)
void screen_touch_test(lv_obj_t *parent);
