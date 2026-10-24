// Study circle: the multiplier and a lamp per friend's device nearby. Friends join over
// ESP-NOW on their own, so there's nothing to choose here. ◂ Back (the knob's only stop) or
// hold goes back.

#include <ui/screens.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>

#include "lvgl.h"

using namespace theme;

static void update_badge(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *badge = lv_observer_get_target_obj(observer);
    lv_label_set_text_fmt(lv_obj_get_child(badge, 0), "%d× XP", (int)lv_subject_get_int(subject));
}

static void update_lamps(lv_observer_t *observer, lv_subject_t *subject)
{
    fill_lamps(lv_observer_get_target_obj(observer), "No study buddies nearby yet.");
}

void screen_circle(lv_obj_t *parent, lv_group_t *group)
{
    AppState &state = AppState::instance();
    lv_obj_t *content = frame(parent);

    Header top = header(content, "Study Circle");
    lv_obj_t *badge = chip(top.row, "", &font_fell_14, true);
    lv_subject_add_observer_obj(&state.base_multiplier, update_badge, badge, nullptr);

    lv_obj_t *lamps = column(content, 8);
    lv_obj_set_width(lamps, LV_PCT(100));
    lv_subject_add_observer_obj(&state.circle_version, update_lamps, lamps, nullptr);

    lv_obj_t *note = label(content, "Each friend nearby adds 1× XP, up to 3×. The bonus pauses while anyone drifts.",
                           &font_caslon_12, PARCHMENT);
    lv_obj_set_width(note, LV_PCT(100));
    lv_label_set_long_mode(note, LV_LABEL_LONG_MODE_WRAP);

    lv_group_add_obj(group, top.back);
}
