// 06 Menu: Begin session, Study circle (N near), {pet}'s workshop, Timer settings.
// Turn to choose, press to open; ◂ Back or hold to go back. Every row also opens on a tap.

#include <ui/screens.hpp>
#include <ui/navigator.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>

#include "lvgl.h"

using namespace theme;

static void update_progress(lv_observer_t *observer, lv_subject_t *subject)
{
    AppState &state = AppState::instance();
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "Lv %d · %d XP",
                          (int)lv_subject_get_int(&state.level), (int)lv_subject_get_int(&state.xp));
}

static void update_lengths(lv_observer_t *observer, lv_subject_t *subject)
{
    AppState &state = AppState::instance();
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "%d / %d",
                          (int)lv_subject_get_int(&state.study_min), (int)lv_subject_get_int(&state.break_min));
}

static void update_near(lv_observer_t *observer, lv_subject_t *subject)
{
    int count = 0;
    AppState::instance().friends(&count);
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "%d near", count);
}

static void update_workshop(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "%s's workshop", lv_subject_get_string(subject));
}

static void on_begin(lv_event_t *e) { AppState::instance().startSession(); }

static void go(lv_event_t *e)
{
    Navigator::instance().navigate((ScreenID)(intptr_t)lv_event_get_user_data(e));
}

void screen_menu(lv_obj_t *parent, lv_group_t *group)
{
    AppState &state = AppState::instance();
    lv_obj_t *content = frame(parent);

    Header top = header(content, "Menu");
    lv_obj_t *progress = label(top.row, "", &font_plex_12);
    lv_subject_add_observer_obj(&state.level, update_progress, progress, nullptr);
    lv_subject_add_observer_obj(&state.xp, update_progress, progress, nullptr);

    lv_obj_t *rows = column(content);
    lv_obj_set_width(rows, LV_PCT(100));

    lv_obj_t *begin = list_row(rows, group, "Begin session", "");
    lv_obj_t *lengths = lv_obj_get_child(begin, 1);
    lv_subject_add_observer_obj(&state.study_min, update_lengths, lengths, nullptr);
    lv_subject_add_observer_obj(&state.break_min, update_lengths, lengths, nullptr);
    lv_obj_add_event_cb(begin, on_begin, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *circle = list_row(rows, group, "Study circle", "", &font_caslon_12, VERDIGRIS_HI);
    lv_subject_add_observer_obj(&state.circle_version, update_near, lv_obj_get_child(circle, 1), nullptr);
    lv_obj_add_event_cb(circle, go, LV_EVENT_CLICKED, (void *)(intptr_t)ScreenID::CIRCLE);

    lv_obj_t *workshop = list_row(rows, group, "");
    lv_obj_t *workshop_text = lv_obj_get_child(lv_obj_get_child(workshop, 0), 1);
    lv_subject_add_observer_obj(&state.pet_name, update_workshop, workshop_text, nullptr);
    lv_obj_add_event_cb(workshop, go, LV_EVENT_CLICKED, (void *)(intptr_t)ScreenID::WORKSHOP);

    lv_obj_t *timer = list_row(rows, group, "Timer settings");
    lv_obj_add_event_cb(timer, go, LV_EVENT_CLICKED, (void *)(intptr_t)ScreenID::TIMER_SETTINGS);

    lv_obj_t *hint = label(content, "Turn to choose · press to open · hold to go back", &font_fell_12, MUTED);
    lv_obj_set_width(hint, LV_PCT(100));
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);

    lv_group_add_obj(group, top.back);
    lv_group_focus_obj(begin);
}
