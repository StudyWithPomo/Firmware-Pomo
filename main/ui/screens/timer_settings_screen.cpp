// Timer settings: default study length, break length (within the allowed range) and
// "Always show timer". Press a length row to change it with the knob, press again to set.
// Press "Always show" to toggle it. Hold goes back.

#include <ui/screens.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>
#include <app/session_core.hpp>

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
    lv_obj_t *r = lv_obj_get_parent(label);
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
        case SET_STUDY: {
            const int current = lv_subject_get_int(&state.study_min);
            int i = 0;
            while (i < SessionCore::STUDY_OPTION_COUNT - 1 && SessionCore::STUDY_OPTIONS[i] < current) i++;
            i += step;
            if (i < 0) i = 0;
            if (i >= SessionCore::STUDY_OPTION_COUNT) i = SessionCore::STUDY_OPTION_COUNT - 1;
            state.setStudyLength(SessionCore::STUDY_OPTIONS[i]);
            break;
        }
        case SET_BREAK:
            state.setBreakLength(lv_subject_get_int(&state.break_min) + step);
            break;
        default: // Always show: turning toggles it too
            lv_subject_set_int(&state.always_show_timer, !lv_subject_get_int(&state.always_show_timer));
            break;
    }
}

static lv_obj_t *setting_row(lv_obj_t *parent, int setting, const char *name)
{
    lv_obj_t *r = list_row(parent, s_group, name, "");
    lv_obj_set_user_data(r, (void *)(intptr_t)setting);
    lv_obj_add_event_cb(r, on_click, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(r, on_key, LV_EVENT_KEY, nullptr);
    return r;
}

void screen_timer_settings(lv_obj_t *parent, lv_group_t *group)
{
    AppState &state = AppState::instance();
    s_group = group;
    lv_obj_t *content = frame(parent);

    label(content, "Timer Settings", &font_fell_16);

    lv_obj_t *rows = column(content);
    lv_obj_set_width(rows, LV_PCT(100));
    lv_obj_t *study = setting_row(rows, SET_STUDY, "Study");
    lv_subject_add_observer_obj(&state.study_min, update_study, lv_obj_get_child(study, 1), nullptr);
    lv_obj_t *brk = setting_row(rows, SET_BREAK, "Break");
    lv_subject_add_observer_obj(&state.break_min, update_study, lv_obj_get_child(brk, 1), nullptr);
    lv_obj_t *always = setting_row(rows, SET_ALWAYS_SHOW, "Always show timer");
    lv_subject_add_observer_obj(&state.always_show_timer, update_always, lv_obj_get_child(always, 1), nullptr);

    lv_obj_t *hint = label(content, "Press to change · hold to go back", &font_fell_12, MUTED);
    lv_obj_set_width(hint, LV_PCT(100));
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);

    lv_group_focus_obj(study);
}
