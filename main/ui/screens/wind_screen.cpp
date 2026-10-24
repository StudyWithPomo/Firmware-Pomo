// 02 Wind the timer: a Study row with nixie digits, a Break row showing the allowed range,
// and Start.
//   Knob: turn moves between Study, Break, Start and ◂ Back. Press a row to grab it (verdigris
//         outline), turn to change it, press again to let go. Hold goes back.
//   Touch: tap a row to select it, − / + change it, tap Start or ◂ Back.

#include <ui/screens.hpp>
#include <ui/navigator.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>
#include <app/session_core.hpp>

#include "lvgl.h"

using namespace theme;

static lv_obj_t *s_hint;

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

static void set_hint(lv_group_t *group)
{
    if (!s_hint) return;
    lv_label_set_text(s_hint, lv_group_get_editing(group) ? "Turn to change · press to set"
                                                          : "Turn to choose · press to grab a row");
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

// A knob press grabs or lets go of the row. A tap has already selected it; − / + do the rest.
static void on_row_click(lv_event_t *e)
{
    if (!from_knob()) return;
    lv_group_t *group = lv_obj_get_group(lv_event_get_current_target_obj(e));
    lv_group_set_editing(group, !lv_group_get_editing(group));
    set_hint(group);
}

static void on_start(lv_event_t *e) { AppState::instance().startSession(); }

static void on_delete(lv_event_t *e) { s_hint = nullptr; }

static lv_obj_t *value_row(lv_obj_t *parent, lv_group_t *group, const char *name, lv_event_cb_t on_key)
{
    lv_obj_t *r = row(parent);
    lv_obj_set_width(r, LV_PCT(100));
    lv_obj_set_style_pad_hor(r, 3, 0);
    lv_obj_set_style_pad_ver(r, 2, 0);
    lv_obj_t *left = row(r, LV_FLEX_ALIGN_START);
    lv_obj_set_width(left, 64);
    lv_obj_t *ptr = pointer(left, &font_fell_16);
    label(left, name, &font_fell_16);
    focusable_outline(r, group, ptr);
    lv_obj_add_event_cb(r, on_key, LV_EVENT_KEY, nullptr);
    lv_obj_add_event_cb(r, on_row_click, LV_EVENT_CLICKED, nullptr);
    return r;
}

void screen_wind(lv_obj_t *parent, lv_group_t *group)
{
    AppState &state = AppState::instance();
    lv_obj_t *content = frame(parent);
    lv_obj_add_event_cb(content, on_delete, LV_EVENT_DELETE, nullptr);

    Header top = header(content, "Wind the Timer");
    lv_obj_t *tag = chip(top.row, "Pomodoro", &font_fell_12);
    lv_subject_add_observer_obj(&state.study_min, update_tag, tag, nullptr);
    lv_subject_add_observer_obj(&state.break_min, update_tag, tag, nullptr);

    lv_obj_t *study = value_row(content, group, "Study", on_study_key);
    const NixieSpec small = { 30, 46, 10, 11, 3, &font_plex_28 };
    lv_obj_t *study_tubes = nixie_row(study, small, true);
    lv_subject_add_observer_obj(&state.study_min, update_study, study_tubes, nullptr);
    step_buttons(study, study);

    lv_obj_t *brk = value_row(content, group, "Break", on_break_key);
    lv_obj_t *middle = column(brk, 0);
    lv_obj_set_flex_align(middle, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *brk_time = label(middle, "", &font_plex_22, VERDIGRIS_HI);
    lv_subject_add_observer_obj(&state.break_min, update_break, brk_time, nullptr);
    lv_obj_t *range = label(middle, "", &font_caslon_12, MUTED);
    lv_subject_add_observer_obj(&state.study_min, update_range, range, nullptr);
    step_buttons(brk, brk);

    lv_obj_t *start = button(content, group, "Start", &font_fell_16);
    lv_obj_set_size(start, LV_PCT(100), 30);
    lv_obj_add_event_cb(start, on_start, LV_EVENT_CLICKED, nullptr);

    s_hint = label(content, "", &font_fell_12, MUTED);
    lv_obj_set_width(s_hint, LV_PCT(100));
    lv_obj_set_style_text_align(s_hint, LV_TEXT_ALIGN_CENTER, 0);

    lv_group_add_obj(group, top.back);
    lv_group_set_focus_cb(group, set_hint); // moving on lets go of a grabbed row
    lv_group_focus_obj(study);
    set_hint(group);
}
