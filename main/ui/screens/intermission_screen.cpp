// 05 Intermission: verdigris frame, the break countdown and the wound-up pet, with Start now
// (skip the rest of the break) and End set. During the long rest after a set there's only
// Finish set. Tapping the pet pets it. When a break runs out the Ready screen asks before
// the next session; the long rest running out ends the set.

#include <ui/screens.hpp>
#include <ui/navigator.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>
#include <ui/pet/pet_view.hpp>
#include <app/session_core.hpp>

#include "lvgl.h"

using namespace theme;

static struct {
    lv_group_t *group;
    lv_obj_t *porthole;
    lv_obj_t *start_now;
    lv_obj_t *end_set;
} ui;

static void update_title(lv_observer_t *observer, lv_subject_t *subject)
{
    AppState &state = AppState::instance();
    const bool long_rest = (Phase)lv_subject_get_int(&state.phase) == Phase::LONG_REST;
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "%s%s", long_rest ? "Long Rest" : "Intermission",
                          lv_subject_get_int(&state.session_test) ? " · Test" : "");
}

static void update_timer(lv_observer_t *observer, lv_subject_t *subject)
{
    char text[8];
    format_mmss(text, sizeof(text), lv_subject_get_int(subject));
    lv_label_set_text(lv_observer_get_target_obj(observer), text);
}

// After the 4th session there's no next one to start early
static void update_buttons(lv_observer_t *observer, lv_subject_t *subject)
{
    const bool long_rest = (Phase)lv_subject_get_int(subject) == Phase::LONG_REST;
    if (long_rest) {
        lv_obj_add_flag(ui.start_now, LV_OBJ_FLAG_HIDDEN);
        if (lv_group_get_focused(ui.group) == ui.start_now) lv_group_focus_obj(ui.end_set);
    } else {
        lv_obj_remove_flag(ui.start_now, LV_OBJ_FLAG_HIDDEN);
    }
    lv_label_set_text(lv_obj_get_child(lv_obj_get_child(ui.end_set, 0), 1), long_rest ? "Finish set" : "End set");
}

static void on_pet_tap(lv_event_t *e) { pet_hearts(ui.porthole); }
static void on_start_now(lv_event_t *e) { AppState::instance().startNextSession(); }
static void on_end_set(lv_event_t *e) { AppState::instance().endSet(); }
static void on_delete(lv_event_t *e) { ui = {}; }

void screen_intermission(lv_obj_t *parent, lv_group_t *group)
{
    AppState &state = AppState::instance();
    ui = {};
    ui.group = group;
    lv_obj_t *content = frame(parent, VERDIGRIS);
    lv_obj_add_event_cb(content, on_delete, LV_EVENT_DELETE, nullptr);

    lv_obj_t *top = row(content);
    lv_obj_set_width(top, LV_PCT(100));
    lv_obj_t *title = label(top, "", &font_fell_16, VERDIGRIS_HI);
    lv_subject_add_observer_obj(&state.phase, update_title, title, nullptr);
    lv_subject_add_observer_obj(&state.session_test, update_title, title, nullptr);
    lv_obj_t *timer = label(top, "", &font_plex_22, NIXIE);
    lv_subject_add_observer_obj(&state.timer_remaining_s, update_timer, timer, nullptr);

    lv_obj_t *middle = row(content, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_width(middle, LV_PCT(100));
    ui.porthole = porthole(middle, 98);
    lv_obj_t *pet = pet_view_create(ui.porthole, 64, PET_MOOD_LOVE);
    lv_obj_add_flag(pet, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(pet, 17); // the whole porthole
    lv_obj_add_event_cb(pet, on_pet_tap, LV_EVENT_PRESSED, nullptr);

    lv_obj_t *buttons = row(content, LV_FLEX_ALIGN_CENTER, 12);
    lv_obj_set_width(buttons, LV_PCT(100));
    lv_obj_set_style_pad_all(buttons, 4, 0); // room for the focus outline
    ui.start_now = button(buttons, group, "Start now", &font_fell_14);
    lv_obj_set_size(ui.start_now, 120, 30);
    lv_obj_add_event_cb(ui.start_now, on_start_now, LV_EVENT_CLICKED, nullptr);
    ui.end_set = button(buttons, group, "End set", &font_fell_14);
    lv_obj_set_size(ui.end_set, 120, 30);
    lv_obj_add_event_cb(ui.end_set, on_end_set, LV_EVENT_CLICKED, nullptr);
    lv_subject_add_observer_obj(&state.phase, update_buttons, content, nullptr);

    // A break has nowhere to go back to: holding the knob does nothing
    Navigator::instance().setHoldHandler(ignore_hold);
    lv_group_focus_obj(lv_obj_has_flag(ui.start_now, LV_OBJ_FLAG_HIDDEN) ? ui.end_set : ui.start_now);
}
