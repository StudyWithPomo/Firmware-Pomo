// Study: everything timer-related on one screen. Study length, break length (within the
// allowed range), "Always show timer", the session log and Start.
//   Knob: turn moves between the rows, Start and ◂ Back. Press a length row to grab it, turn
//         to change it, press again to set. Press "Always show" to toggle it. Hold goes back.
//   Touch: − / + on the length rows, tap "Always show" to toggle it, Start, ◂ Back.
// The "Test" chip in the corner switches on test timing (30 s phases) for trying the flow;
// it's off again after a restart.
// Opened from the Circle screen to invite a friend, Start reads "Invite Sidak": it sends the
// invite with these lengths and goes back to the Circle. Leaving any other way sends nothing.

#include <ui/screens.hpp>
#include <ui/navigator.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>
#include <app/session_core.hpp>
#include <comms/circle.hpp>

#include "lvgl.h"

using namespace theme;

enum Setting { SET_STUDY, SET_BREAK, SET_ALWAYS_SHOW };

static lv_group_t *s_group;

static void show_value(lv_obj_t *label, const char *text, bool editing)
{
    if (editing) lv_label_set_text_fmt(label, "< %s >", text);
    else lv_label_set_text(label, text);
}

static bool row_is_editing(lv_obj_t *label)
{
    lv_obj_t *r = lv_obj_get_parent(lv_obj_get_parent(label));
    return lv_group_get_editing(s_group) && lv_group_get_focused(s_group) == r;
}

static void update_study(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *label = lv_observer_get_target_obj(observer);
    char text[12];
    lv_snprintf(text, sizeof(text), "%d min", (int)lv_subject_get_int(subject));
    show_value(label, text, row_is_editing(label));
}

static void update_always(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_label_set_text(lv_observer_get_target_obj(observer), lv_subject_get_int(subject) ? "On" : "Off");
}

static void update_today(lv_observer_t *observer, lv_subject_t *subject)
{
    AppState &state = AppState::instance();
    lv_obj_t *label = lv_observer_get_target_obj(observer);
    if (state.today() == 0) lv_label_set_text(label, "Today  –  (clock not set)");
    else lv_label_set_text_fmt(label, "Today  %d session%s", state.sessionsToday(), state.sessionsToday() == 1 ? "" : "s");
}

static void update_totals(lv_observer_t *observer, lv_subject_t *subject)
{
    const SessionLog &log = AppState::instance().log();
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "All time  %d session%s · %d set%s · %dh %02dm focused",
                          log.sessions, log.sessions == 1 ? "" : "s", log.sets, log.sets == 1 ? "" : "s",
                          log.focused_min / 60, log.focused_min % 60);
}

static void on_start(lv_event_t *e)
{
    AppState &state = AppState::instance();
    const AppState::InviteTarget target = state.inviteTarget();
    if (!target.active) {
        state.startSession();
        return;
    }
    circle_invite(target.mac, lv_subject_get_int(&state.study_min), lv_subject_get_int(&state.break_min),
                  lv_subject_get_int(&state.test_timing) != 0);
    Navigator::instance().back();
}

static void on_delete(lv_event_t *e) { AppState::instance().setInviteTarget(nullptr); }

static void on_test(lv_event_t *e)
{
    lv_subject_t *test = &AppState::instance().test_timing;
    lv_subject_set_int(test, !lv_subject_get_int(test));
}

static void update_test_chip(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *chip_obj = lv_observer_get_target_obj(observer);
    const bool on = lv_subject_get_int(subject);
    lv_label_set_text(lv_obj_get_child(chip_obj, 0), on ? "Test: 30 s" : "Test: off");
    lv_obj_set_style_border_color(chip_obj, c(on ? NIXIE : MUTED), 0);
    lv_obj_set_style_text_color(lv_obj_get_child(chip_obj, 0), c(on ? NIXIE : MUTED), 0);
}

static void update_start(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *start = lv_observer_get_target_obj(observer);
    lv_obj_t *text = lv_obj_get_child(lv_obj_get_child(start, 0), 1);
    const AppState::InviteTarget &target = AppState::instance().inviteTarget();
    const bool test = lv_subject_get_int(subject);
    if (target.active) lv_label_set_text_fmt(text, "Invite %s%s", target.name, test ? " · test run" : "");
    else lv_label_set_text(text, test ? "Start test run" : "Start");
}

// Re-render a row's value (to add or drop the "< >" edit marks)
static void refresh(lv_obj_t *r)
{
    AppState &state = AppState::instance();
    const int setting = (int)(intptr_t)lv_obj_get_user_data(r);
    if (setting == SET_STUDY) lv_subject_notify(&state.study_min);
    else if (setting == SET_BREAK) lv_subject_notify(&state.break_min);
}

static void on_click(lv_event_t *e)
{
    lv_obj_t *r = lv_event_get_current_target_obj(e);
    AppState &state = AppState::instance();
    const int setting = (int)(intptr_t)lv_obj_get_user_data(r);
    if (setting == SET_ALWAYS_SHOW) {
        lv_subject_set_int(&state.always_show_timer, !lv_subject_get_int(&state.always_show_timer));
        return;
    }
    if (!from_knob()) return; // a tap only selects the row; − / + change it
    lv_group_set_editing(s_group, !lv_group_get_editing(s_group));
    refresh(r);
}

static void on_key(lv_event_t *e)
{
    const uint32_t key = lv_event_get_key(e);
    if (key != LV_KEY_RIGHT && key != LV_KEY_LEFT) return;
    lv_obj_t *r = lv_event_get_current_target_obj(e);
    AppState &state = AppState::instance();
    const int step = key == LV_KEY_RIGHT ? 1 : -1;

    switch ((int)(intptr_t)lv_obj_get_user_data(r)) {
        case SET_STUDY: // 15-90 in steps of 5; setStudyLength clamps the ends
            state.setStudyLength(lv_subject_get_int(&state.study_min) + step * SessionCore::STUDY_STEP);
            break;
        case SET_BREAK:
            state.setBreakLength(lv_subject_get_int(&state.break_min) + step);
            break;
        default: // Always show: turning toggles it too
            lv_subject_set_int(&state.always_show_timer, !lv_subject_get_int(&state.always_show_timer));
            break;
    }
}

// Returns the value label: "[▸] Name ...... value [− +]"
static lv_obj_t *setting_row(lv_obj_t *parent, int setting, const char *name, bool steps)
{
    lv_obj_t *r = list_row(parent, s_group, name, "");
    lv_obj_set_height(r, 29);
    lv_obj_set_user_data(r, (void *)(intptr_t)setting);
    lv_obj_add_event_cb(r, on_click, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(r, on_key, LV_EVENT_KEY, nullptr);

    lv_obj_t *right = row(r, LV_FLEX_ALIGN_START, 10);
    lv_obj_t *value = lv_obj_get_child(r, 1);
    lv_obj_set_parent(value, right);
    if (steps) step_buttons(right, r);
    return value;
}

static void on_group_focus(lv_group_t *group)
{
    // Moving on lets go of a grabbed row: redraw every value without its "< >"
    AppState &state = AppState::instance();
    lv_subject_notify(&state.study_min);
    lv_subject_notify(&state.break_min);
}

void screen_study(lv_obj_t *parent, lv_group_t *group)
{
    AppState &state = AppState::instance();
    s_group = group;
    lv_obj_t *content = frame(parent);
    lv_obj_add_event_cb(content, on_delete, LV_EVENT_DELETE, nullptr);

    Header top = header(content, "Study");
    lv_obj_t *test = chip(top.row, "", &font_fell_12);
    lv_obj_set_style_pad_ver(test, 2, 0);
    lv_obj_add_event_cb(test, on_test, LV_EVENT_CLICKED, nullptr);
    lv_subject_add_observer_obj(&state.test_timing, update_test_chip, test, nullptr);

    lv_obj_t *rows = column(content);
    lv_obj_set_width(rows, LV_PCT(100));
    lv_obj_t *study = setting_row(rows, SET_STUDY, "Study", true);
    lv_subject_add_observer_obj(&state.study_min, update_study, study, nullptr);
    lv_obj_t *brk = setting_row(rows, SET_BREAK, "Break", true);
    lv_subject_add_observer_obj(&state.break_min, update_study, brk, nullptr);
    lv_obj_t *always = setting_row(rows, SET_ALWAYS_SHOW, "Always show timer", false);
    lv_subject_add_observer_obj(&state.always_show_timer, update_always, always, nullptr);

    lv_obj_t *log = column(content, 2);
    lv_obj_set_width(log, LV_PCT(100));
    lv_obj_set_style_pad_hor(log, 9, 0);
    lv_obj_t *today = label(log, "", &font_caslon_12, PARCHMENT);
    lv_subject_add_observer_obj(&state.log_version, update_today, today, nullptr);
    lv_subject_add_observer_obj(&state.time_subject, update_today, today, nullptr);
    lv_obj_t *totals = label(log, "", &font_caslon_12, MUTED);
    lv_subject_add_observer_obj(&state.log_version, update_totals, totals, nullptr);

    lv_obj_t *start = button(content, group, "Start", &font_fell_16);
    lv_obj_set_size(start, LV_PCT(100), 30);
    lv_obj_add_event_cb(start, on_start, LV_EVENT_CLICKED, nullptr);
    lv_subject_add_observer_obj(&state.test_timing, update_start, start, nullptr);

    focusable(test, group); // after Start, before Back
    lv_group_add_obj(group, top.back);
    lv_group_set_focus_cb(group, on_group_focus);
    lv_group_focus_obj(lv_obj_get_parent(lv_obj_get_parent(study))); // the Study row
}
