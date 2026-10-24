// 03 Focus, with 03b Peek and 04 Drift as states of the same screen. All three views are
// built once; observers switch between them and update the timer, so nothing is rebuilt
// every second.
//   Dark (default): pet centre stage, unlit tubes, "Press to peek"
//   Peek:           tubes lit for 5 s (or always, with "Always show timer"), progress rail
//   Drift:          copper band naming the drift, annoyed pet, "2x paused"
// Press = peek. Hold = end the session early, after a confirm. Touch is locked, except that
// tapping the pet hushes it.

#include <stdio.h>

#include <ui/screens.hpp>
#include <ui/navigator.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>
#include <ui/pet/pet_view.hpp>

#include "lvgl.h"

using namespace theme;

static struct {
    lv_group_t *group;
    lv_obj_t *target;   // the knob's only stop: receives press-to-peek
    lv_obj_t *dark;
    lv_obj_t *peek;
    lv_obj_t *drift;
    lv_obj_t *modal;
} ui;

static void show_only(lv_obj_t *view)
{
    lv_obj_t *views[] = { ui.dark, ui.peek, ui.drift };
    for (lv_obj_t *v : views) {
        if (v == view) lv_obj_remove_flag(v, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(v, LV_OBJ_FLAG_HIDDEN);
    }
}

static void update_view(lv_observer_t *observer, lv_subject_t *subject)
{
    AppState &state = AppState::instance();
    if (lv_subject_get_int(&state.focus_state)) show_only(ui.drift);
    else if (lv_subject_get_int(&state.peek_active) || lv_subject_get_int(&state.always_show_timer)) show_only(ui.peek);
    else show_only(ui.dark);
}

// ---- Observers ----

static void update_badge(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *badge = lv_observer_get_target_obj(observer);
    lv_label_set_text_fmt(lv_obj_get_child(badge, 0), "%d× XP", (int)lv_subject_get_int(subject));
}

static void update_nixie(lv_observer_t *observer, lv_subject_t *subject)
{
    nixie_set(lv_observer_get_target_obj(observer), lv_subject_get_int(subject));
}

static void update_rail(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *fill = lv_observer_get_target_obj(observer);
    AppState &state = AppState::instance();
    const int total = lv_subject_get_int(&state.timer_total_s);
    const int left = lv_subject_get_int(&state.timer_remaining_s);
    const int pct = total > 0 ? (total - left) * 100 / total : 0;
    lv_obj_set_width(fill, LV_PCT(pct));
}

static void update_small_timer(lv_observer_t *observer, lv_subject_t *subject)
{
    char text[8];
    format_mmss(text, sizeof(text), lv_subject_get_int(subject));
    lv_label_set_text(lv_observer_get_target_obj(observer), text);
}

static void update_drift_name(lv_observer_t *observer, lv_subject_t *subject)
{
    const char *name = lv_subject_get_string(subject);
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "%s Detected", name[0] ? name : "Drift");
}

static void update_drift_time(lv_observer_t *observer, lv_subject_t *subject)
{
    const int s = lv_subject_get_int(subject);
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "%d:%02d", s / 60, s % 60);
}

static void update_paused(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *label = lv_observer_get_target_obj(observer);
    const int base = lv_subject_get_int(subject);
    lv_label_set_text_fmt(label, "%d× paused", base);
    if (base > 1) lv_obj_remove_flag(label, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
}

static void update_mood(lv_observer_t *observer, lv_subject_t *subject)
{
    pet_view_set_mood(lv_observer_get_target_obj(observer), (PetMood)lv_subject_get_int(subject));
}

static void update_lamps(lv_observer_t *observer, lv_subject_t *subject)
{
    fill_lamps(lv_observer_get_target_obj(observer));
}

// ---- Input ----

static void on_press(lv_event_t *e)
{
    if (!ui.modal) AppState::instance().peek();
}

static void on_pet_tap(lv_event_t *e)
{
    if (!lv_subject_get_int(&AppState::instance().focus_state)) AppState::instance().shh();
}

static void close_modal()
{
    if (!ui.modal) return;
    lv_obj_delete_async(ui.modal); // its buttons leave the group as they're deleted
    ui.modal = nullptr;
    lv_group_add_obj(ui.group, ui.target);
    lv_group_focus_obj(ui.target);
}

static void on_keep_going(lv_event_t *e) { close_modal(); }
static void on_end_it(lv_event_t *e) { AppState::instance().endSessionEarly(); }

static void confirm_end()
{
    if (ui.modal) return;
    lv_obj_t *content = lv_obj_get_parent(ui.dark);
    ui.modal = box(content);
    lv_obj_add_flag(ui.modal, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(ui.modal, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_size(ui.modal, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(ui.modal, LV_OPA_90, 0);
    lv_obj_set_style_bg_color(ui.modal, c(PANEL), 0);
    lv_obj_set_flex_flow(ui.modal, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui.modal, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(ui.modal, 14, 0);

    label(ui.modal, "End session early?", &font_fell_16, BRASS_HI);
    lv_obj_t *buttons = row(ui.modal, LV_FLEX_ALIGN_CENTER, 10);

    lv_group_remove_obj(ui.target);
    lv_obj_t *keep = button(buttons, ui.group, "Keep going", &font_fell_14);
    lv_obj_set_size(keep, 120, 32);
    lv_obj_add_event_cb(keep, on_keep_going, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *end = button(buttons, ui.group, "End it", &font_fell_14);
    lv_obj_set_size(end, 100, 32);
    lv_obj_add_event_cb(end, on_end_it, LV_EVENT_CLICKED, nullptr);
    lv_group_focus_obj(keep);
}

// ---- Layout ----

static lv_obj_t *view(lv_obj_t *content)
{
    lv_obj_t *v = box(content);
    lv_obj_set_size(v, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_top(v, 7, 0);
    lv_obj_set_style_pad_bottom(v, 9, 0);
    lv_obj_set_style_pad_hor(v, 9, 0);
    lv_obj_set_flex_flow(v, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(v, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    return v;
}

static void header(lv_obj_t *v)
{
    lv_obj_t *top = row(v);
    lv_obj_set_width(top, LV_PCT(100));
    label(top, "In Session", &font_fell_14);
    lv_obj_t *badge = chip(top, "", &font_fell_14, true);
    lv_subject_add_observer_obj(&AppState::instance().multiplier, update_badge, badge, nullptr);
}

static lv_obj_t *focus_pet(lv_obj_t *parent, int size)
{
    lv_obj_t *pet = pet_view_create(parent, size, PET_MOOD_FOCUS);
    lv_obj_add_flag(pet, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(pet, on_pet_tap, LV_EVENT_PRESSED, nullptr);
    lv_subject_add_observer_obj(&AppState::instance().pet_mood, update_mood, pet, nullptr);
    return pet;
}

static lv_obj_t *lamp_list(lv_obj_t *parent, bool vertical)
{
    lv_obj_t *lamps = vertical ? column(parent, 3) : row(parent, LV_FLEX_ALIGN_SPACE_BETWEEN, 12);
    if (!vertical) lv_obj_set_width(lamps, LV_PCT(100));
    lv_subject_add_observer_obj(&AppState::instance().circle_version, update_lamps, lamps, nullptr);
    return lamps;
}

static void build_dark(lv_obj_t *v)
{
    header(v);

    lv_obj_t *middle = row(v, LV_FLEX_ALIGN_CENTER, 14);
    lv_obj_set_width(middle, LV_PCT(100));
    lv_obj_t *port = porthole(middle, 94);
    focus_pet(port, 64);

    lv_obj_t *side = column(middle, 7);
    lv_obj_set_flex_align(side, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    const NixieSpec dark = { 26, 42, 7, 9, 3, &font_plex_22 };
    nixie_row(side, dark, false);
    label(side, "Press to peek", &font_fell_14, BRASS_HI);

    lamp_list(v, false);
}

static void build_peek(lv_obj_t *v)
{
    AppState &state = AppState::instance();
    header(v);

    lv_obj_t *middle = column(v, 7);
    lv_obj_set_width(middle, LV_PCT(100));
    lv_obj_set_flex_align(middle, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    const NixieSpec big = { 48, 70, 18, 16, 5, &font_plex_50 };
    lv_obj_t *tubes = nixie_row(middle, big, true);
    lv_subject_add_observer_obj(&state.timer_remaining_s, update_nixie, tubes, nullptr);

    lv_obj_t *rail = box(middle);
    lv_obj_set_size(rail, 240, 7);
    lv_obj_set_style_border_width(rail, 1, 0);
    lv_obj_set_style_border_color(rail, c(BRASS), 0);
    lv_obj_set_style_pad_all(rail, 1, 0);
    lv_obj_t *fill = box(rail);
    lv_obj_set_height(fill, LV_PCT(100));
    lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(fill, c(BRASS), 0);
    lv_subject_add_observer_obj(&state.timer_remaining_s, update_rail, fill, nullptr);

    lv_obj_t *bottom = row(v);
    lv_obj_set_width(bottom, LV_PCT(100));
    lv_obj_t *port = porthole(bottom, 52, 2);
    focus_pet(port, 32);
    lamp_list(bottom, true);
}

static void build_drift(lv_obj_t *v)
{
    AppState &state = AppState::instance();
    lv_obj_set_style_pad_all(v, 0, 0);
    lv_obj_set_style_pad_bottom(v, 7, 0);

    lv_obj_t *band = row(v);
    lv_obj_set_size(band, LV_PCT(100), 32);
    lv_obj_set_style_bg_opa(band, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(band, c(COPPER), 0);
    lv_obj_set_style_pad_hor(band, 9, 0);
    lv_obj_t *name = label(band, "", &font_fell_16, ALARM_TEXT);
    lv_subject_add_observer_obj(&state.drift_label, update_drift_name, name, nullptr);
    lv_obj_t *drift_time = label(band, "", &font_fell_16, ALARM_TEXT);
    lv_subject_add_observer_obj(&state.drift_seconds, update_drift_time, drift_time, nullptr);

    lv_obj_t *middle = row(v, LV_FLEX_ALIGN_CENTER, 14);
    lv_obj_set_width(middle, LV_PCT(100));
    lv_obj_t *port = porthole(middle, 90);
    pet_view_create(port, 64, PET_MOOD_ANNOYED);
    bubble(middle, "Kindly return to your studies.", 115);

    lv_obj_t *bottom = row(v);
    lv_obj_set_width(bottom, LV_PCT(100));
    lv_obj_set_style_pad_hor(bottom, 9, 0);
    lv_obj_t *timer = label(bottom, "", &font_plex_28, NIXIE);
    lv_subject_add_observer_obj(&state.timer_remaining_s, update_small_timer, timer, nullptr);
    lv_obj_t *paused = label(bottom, "", &font_fell_14, PAUSED);
    lv_subject_add_observer_obj(&state.base_multiplier, update_paused, paused, nullptr);
}

static void on_screen_delete(lv_event_t *e)
{
    ui = {};
}

void screen_focus(lv_obj_t *parent, lv_group_t *group)
{
    AppState &state = AppState::instance();
    ui = {};
    ui.group = group;

    lv_obj_t *content = frame(parent);
    lv_obj_set_style_pad_all(content, 0, 0); // each view pads itself; Drift's band is flush
    lv_obj_add_event_cb(content, on_screen_delete, LV_EVENT_DELETE, nullptr);

    ui.dark = view(content);
    build_dark(ui.dark);
    ui.peek = view(content);
    build_peek(ui.peek);
    ui.drift = view(content);
    build_drift(ui.drift);

    // The knob's single stop. Not clickable, so touch can't peek or end the session.
    ui.target = content;
    lv_group_add_obj(group, content);
    lv_obj_add_event_cb(content, on_press, LV_EVENT_CLICKED, nullptr);
    Navigator::instance().setHoldHandler(confirm_end);

    lv_subject_add_observer_obj(&state.focus_state, update_view, content, nullptr);
    lv_subject_add_observer_obj(&state.peek_active, update_view, content, nullptr);
    lv_subject_add_observer_obj(&state.always_show_timer, update_view, content, nullptr);
}
