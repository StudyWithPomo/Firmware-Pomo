// 02 Wind the timer: a Study row with nixie digits and a Break row showing the allowed range.
// Turn changes the focused row's value; press moves from Study to Break, then starts.
// Hold goes back.

#include <ui/screens.hpp>
#include <ui/navigator.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>
#include <app/session_core.hpp>

#include "lvgl.h"

using namespace theme;

static lv_obj_t *s_break_row;

static void update_tag(lv_observer_t *observer, lv_subject_t *subject)
{
    AppState &state = AppState::instance();
    const bool classic = lv_subject_get_int(&state.study_min) == 25 && lv_subject_get_int(&state.break_min) == 5;
    lv_obj_t *tag = lv_observer_get_target_obj(observer);
    lv_label_set_text(lv_obj_get_child(tag, 0), classic ? "Pomodoro" : "Custom");
}

static void update_study(lv_observer_t *observer, lv_subject_t *subject)
{
    nixie_set(lv_observer_get_target_obj(observer), lv_subject_get_int(subject) * 60);
}

static void update_break(lv_observer_t *observer, lv_subject_t *subject)
{
    char text[8];
    format_mmss(text, sizeof(text), lv_subject_get_int(&AppState::instance().break_min) * 60);
    lv_label_set_text(lv_observer_get_target_obj(observer), text);
}

static void update_range(lv_observer_t *observer, lv_subject_t *subject)
{
    const int study = lv_subject_get_int(subject);
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "allowed %d–%d",
                          SessionCore::breakMin(study), SessionCore::breakMax(study));
}

static void on_study_key(lv_event_t *e)
{
    const uint32_t key = lv_event_get_key(e);
    if (key != LV_KEY_RIGHT && key != LV_KEY_LEFT) return;
    AppState &state = AppState::instance();
    const int current = lv_subject_get_int(&state.study_min);
    int i = 0;
    while (i < SessionCore::STUDY_OPTION_COUNT - 1 && SessionCore::STUDY_OPTIONS[i] < current) i++;
    i += key == LV_KEY_RIGHT ? 1 : -1;
    if (i < 0) i = 0;
    if (i >= SessionCore::STUDY_OPTION_COUNT) i = SessionCore::STUDY_OPTION_COUNT - 1;
    state.setStudyLength(SessionCore::STUDY_OPTIONS[i]);
}

static void on_break_key(lv_event_t *e)
{
    const uint32_t key = lv_event_get_key(e);
    if (key != LV_KEY_RIGHT && key != LV_KEY_LEFT) return;
    AppState &state = AppState::instance();
    state.setBreakLength(lv_subject_get_int(&state.break_min) + (key == LV_KEY_RIGHT ? 1 : -1));
}

static void on_study_click(lv_event_t *e) { lv_group_focus_obj(s_break_row); }
static void on_break_click(lv_event_t *e) { AppState::instance().startSession(); }

static lv_obj_t *value_row(lv_obj_t *parent, lv_group_t *group, const char *name)
{
    lv_obj_t *r = row(parent);
    lv_obj_set_width(r, LV_PCT(100));
    lv_obj_set_style_pad_hor(r, 3, 0);
    lv_obj_set_style_pad_ver(r, 2, 0);
    lv_obj_t *left = row(r, LV_FLEX_ALIGN_START);
    lv_obj_t *ptr = pointer(left, &font_fell_16);
    label(left, name, &font_fell_16);
    focusable_outline(r, group, ptr);
    return r;
}

void screen_wind(lv_obj_t *parent, lv_group_t *group)
{
    AppState &state = AppState::instance();
    lv_obj_t *content = frame(parent);

    lv_obj_t *top = row(content);
    lv_obj_set_width(top, LV_PCT(100));
    label(top, "Wind the Timer", &font_fell_16);
    lv_obj_t *tag = chip(top, "Pomodoro", &font_fell_12);
    lv_subject_add_observer_obj(&state.study_min, update_tag, tag, nullptr);
    lv_subject_add_observer_obj(&state.break_min, update_tag, tag, nullptr);

    lv_obj_t *study = value_row(content, group, "Study");
    const NixieSpec small = { 32, 50, 12, 12, 3, &font_plex_28 };
    lv_obj_t *study_tubes = nixie_row(study, small, true);
    lv_subject_add_observer_obj(&state.study_min, update_study, study_tubes, nullptr);
    lv_obj_add_event_cb(study, on_study_key, LV_EVENT_KEY, nullptr);
    lv_obj_add_event_cb(study, on_study_click, LV_EVENT_CLICKED, nullptr);

    s_break_row = value_row(content, group, "Break");
    lv_obj_t *right = row(s_break_row, LV_FLEX_ALIGN_START, 8);
    lv_obj_t *brk = label(right, "", &font_plex_22, VERDIGRIS_HI);
    lv_subject_add_observer_obj(&state.break_min, update_break, brk, nullptr);
    lv_obj_t *range = label(right, "", &font_caslon_12, MUTED);
    lv_subject_add_observer_obj(&state.study_min, update_range, range, nullptr);
    lv_obj_add_event_cb(s_break_row, on_break_key, LV_EVENT_KEY, nullptr);
    lv_obj_add_event_cb(s_break_row, on_break_click, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *hint = label(content, "Turn to set · press for next · hold to go back", &font_fell_12, MUTED);
    lv_obj_set_width(hint, LV_PCT(100));
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);

    // Turning always changes the focused value; presses move between rows
    lv_group_focus_obj(study);
    always_editing(group);
}
