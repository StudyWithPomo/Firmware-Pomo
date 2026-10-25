// 03 Focus, with 03b Peek and 04 Drift as states of the same screen. All three views are
// built once; observers switch between them and update the timer, so nothing is rebuilt
// every second.
//   Dark (default): pet centre stage, unlit tubes
//   Peek:           tubes lit for 5 s (or always, with "Always show timer"), progress rail
//   Drift:          copper band naming the drift, annoyed pet, "2x paused"
//   Laptop lost:    the laptop's heartbeat stopped, so the timer is held (as the dashboard
//                   says too); the sad pet waits for it, or Go on alone carries on without it
// Under the views, Peek and End session buttons for the knob and touch alike. Peek is hidden
// while drifting or always showing the timer, and becomes Go on alone while the laptop is
// lost. End session (or a 2 s hold) asks first. Tapping the pet hushes it.

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
    lv_obj_t *peek_btn;
    lv_obj_t *end_btn;
    lv_obj_t *dark;
    lv_obj_t *peek;
    lv_obj_t *drift;
    lv_obj_t *lost;
    lv_obj_t *modal;
} ui;

static void show_only(lv_obj_t *view)
{
    lv_obj_t *views[] = { ui.dark, ui.peek, ui.drift, ui.lost };
    for (lv_obj_t *v : views) {
        if (v == view) lv_obj_remove_flag(v, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(v, LV_OBJ_FLAG_HIDDEN);
    }
}

static void update_view(lv_observer_t *observer, lv_subject_t *subject)
{
    AppState &state = AppState::instance();
    const bool paused = lv_subject_get_int(&state.session_paused);
    const bool drifting = lv_subject_get_int(&state.focus_state);
    const bool always = lv_subject_get_int(&state.always_show_timer);
    if (paused) show_only(ui.lost);
    else if (drifting) show_only(ui.drift);
    else if (lv_subject_get_int(&state.peek_active) || always) show_only(ui.peek);
    else show_only(ui.dark);

    lv_label_set_text(lv_obj_get_child(lv_obj_get_child(ui.peek_btn, 0), 1), paused ? "Go on alone" : "Peek");
    // Nothing to peek at: the knob skips the hidden button
    if (!paused && (drifting || always)) {
        lv_obj_add_flag(ui.peek_btn, LV_OBJ_FLAG_HIDDEN);
        if (lv_group_get_focused(ui.group) == ui.peek_btn) lv_group_focus_obj(ui.end_btn);
    } else {
        lv_obj_remove_flag(ui.peek_btn, LV_OBJ_FLAG_HIDDEN);
    }
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

// In the dashboard's words: "YouTube Detected", "Gone Idle", or what the camera saw
static void update_drift_name(lv_observer_t *observer, lv_subject_t *subject)
{
    AppState &state = AppState::instance();
    const char *name = lv_subject_get_string(&state.drift_label);
    lv_obj_t *label = lv_observer_get_target_obj(observer);
    switch ((DriftKind)lv_subject_get_int(&state.drift_kind)) {
        case DriftKind::IDLE: lv_label_set_text(label, "Gone Idle"); break;
        case DriftKind::CAMERA: lv_label_set_text(label, name[0] ? name : "Camera"); break;
        default: lv_label_set_text_fmt(label, "%s Detected", name[0] ? name : "Drift"); break;
    }
}

static void update_drift_note(lv_observer_t *observer, lv_subject_t *subject)
{
    AppState &state = AppState::instance();
    lv_obj_t *text = lv_obj_get_child(lv_observer_get_target_obj(observer), 0);
    if ((DriftKind)lv_subject_get_int(&state.drift_kind) == DriftKind::IDLE)
        lv_label_set_text_fmt(text, "%s is waiting for you.", lv_subject_get_string(&state.pet_name));
    else
        lv_label_set_text(text, "Kindly return to your studies.");
}

static void update_earned(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "+%d XP so far", (int)lv_subject_get_int(subject));
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

static void on_peek(lv_event_t *e)
{
    if (ui.modal) return;
    AppState &state = AppState::instance();
    if (lv_subject_get_int(&state.session_paused)) state.goOnAlone();
    else state.peek();
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
    lv_group_add_obj(ui.group, ui.peek_btn);
    lv_group_add_obj(ui.group, ui.end_btn);
    lv_group_focus_obj(lv_obj_has_flag(ui.peek_btn, LV_OBJ_FLAG_HIDDEN) ? ui.end_btn : ui.peek_btn);
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
    lv_obj_add_flag(ui.modal, LV_OBJ_FLAG_CLICKABLE); // catches taps meant for what's under it
    lv_obj_set_style_bg_opa(ui.modal, LV_OPA_90, 0);
    lv_obj_set_style_bg_color(ui.modal, c(PANEL), 0);
    lv_obj_set_flex_flow(ui.modal, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui.modal, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(ui.modal, 14, 0);

    label(ui.modal, "End session early?", &font_fell_16, BRASS_HI);
    lv_obj_t *buttons = row(ui.modal, LV_FLEX_ALIGN_CENTER, 10);

    lv_group_remove_obj(ui.peek_btn);
    lv_group_remove_obj(ui.end_btn);
    lv_obj_t *keep = button(buttons, ui.group, "Keep going", &font_fell_14);
    lv_obj_set_size(keep, 120, 32);
    lv_obj_add_event_cb(keep, on_keep_going, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *end = button(buttons, ui.group, "End it", &font_fell_14);
    lv_obj_set_size(end, 100, 32);
    lv_obj_add_event_cb(end, on_end_it, LV_EVENT_CLICKED, nullptr);
    lv_group_focus_obj(keep);
}

static void on_end(lv_event_t *e) { confirm_end(); }

// ---- Layout ----

// A view fills the space above the button bar
static lv_obj_t *view(lv_obj_t *content)
{
    lv_obj_t *v = box(content);
    lv_obj_set_width(v, LV_PCT(100));
    lv_obj_set_flex_grow(v, 1);
    lv_obj_set_style_pad_top(v, 7, 0);
    lv_obj_set_style_pad_bottom(v, 2, 0);
    lv_obj_set_style_pad_hor(v, 9, 0);
    lv_obj_set_flex_flow(v, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(v, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    return v;
}

// "Session II of IV", as the dashboard numbers them
static void update_title(lv_observer_t *observer, lv_subject_t *subject)
{
    AppState &state = AppState::instance();
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "Session %s of IV%s",
                          roman(lv_subject_get_int(&state.session_index)),
                          lv_subject_get_int(&state.session_test) ? " · Test" : "");
}

static void header(lv_obj_t *v)
{
    lv_obj_t *top = row(v);
    lv_obj_set_width(top, LV_PCT(100));
    lv_obj_t *title = label(top, "", &font_fell_14);
    lv_subject_add_observer_obj(&AppState::instance().session_test, update_title, title, nullptr);
    lv_subject_add_observer_obj(&AppState::instance().session_index, update_title, title, nullptr);
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
    lv_obj_t *earned = label(middle, "", &font_caslon_12, MUTED);
    lv_subject_add_observer_obj(&state.earned, update_earned, earned, nullptr);

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

    lv_obj_t *band = row(v);
    lv_obj_set_size(band, LV_PCT(100), 32);
    lv_obj_set_style_bg_opa(band, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(band, c(COPPER), 0);
    lv_obj_set_style_pad_hor(band, 9, 0);
    lv_obj_t *name = label(band, "", &font_fell_16, ALARM_TEXT);
    lv_subject_add_observer_obj(&state.drift_label, update_drift_name, name, nullptr);
    lv_subject_add_observer_obj(&state.drift_kind, update_drift_name, name, nullptr);
    lv_obj_t *drift_time = label(band, "", &font_fell_16, ALARM_TEXT);
    lv_subject_add_observer_obj(&state.drift_seconds, update_drift_time, drift_time, nullptr);

    lv_obj_t *middle = row(v, LV_FLEX_ALIGN_CENTER, 14);
    lv_obj_set_width(middle, LV_PCT(100));
    lv_obj_t *port = porthole(middle, 90);
    pet_view_create(port, 64, PET_MOOD_ANNOYED);
    lv_obj_t *note = bubble(middle, "", 115);
    lv_subject_add_observer_obj(&state.drift_kind, update_drift_note, note, nullptr);

    lv_obj_t *bottom = row(v);
    lv_obj_set_width(bottom, LV_PCT(100));
    lv_obj_set_style_pad_hor(bottom, 9, 0);
    lv_obj_t *timer = label(bottom, "", &font_plex_28, NIXIE);
    lv_subject_add_observer_obj(&state.timer_remaining_s, update_small_timer, timer, nullptr);
    lv_obj_t *paused = label(bottom, "", &font_fell_14, PAUSED);
    lv_subject_add_observer_obj(&state.base_multiplier, update_paused, paused, nullptr);
}

static void build_lost(lv_obj_t *v)
{
    AppState &state = AppState::instance();
    lv_obj_set_style_pad_all(v, 0, 0);

    lv_obj_t *band = row(v);
    lv_obj_set_size(band, LV_PCT(100), 32);
    lv_obj_set_style_bg_opa(band, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(band, c(TUBE_RIM), 0);
    lv_obj_set_style_pad_hor(band, 9, 0);
    label(band, "Laptop Lost", &font_fell_16, BRASS_HI);
    label(band, "Paused", &font_fell_16, BRASS_HI);

    lv_obj_t *middle = row(v, LV_FLEX_ALIGN_CENTER, 14);
    lv_obj_set_width(middle, LV_PCT(100));
    lv_obj_t *port = porthole(middle, 90);
    pet_view_create(port, 64, PET_MOOD_SAD);
    bubble(middle, "The timer waits. Plug the laptop back in, or go on alone.", 130);

    lv_obj_t *bottom = row(v);
    lv_obj_set_width(bottom, LV_PCT(100));
    lv_obj_set_style_pad_hor(bottom, 9, 0);
    lv_obj_t *timer = label(bottom, "", &font_plex_28, TUBE_RIM);
    lv_subject_add_observer_obj(&state.timer_remaining_s, update_small_timer, timer, nullptr);
    label(bottom, "timer held", &font_fell_14, MUTED);
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
    ui.lost = view(content);
    build_lost(ui.lost);

    lv_obj_t *bar = row(content, LV_FLEX_ALIGN_CENTER, 12);
    lv_obj_set_width(bar, LV_PCT(100));
    lv_obj_set_style_pad_top(bar, 2, 0);
    lv_obj_set_style_pad_bottom(bar, 8, 0);
    ui.peek_btn = button(bar, group, "Peek", &font_fell_14);
    lv_obj_set_size(ui.peek_btn, 110, 26);
    lv_obj_add_event_cb(ui.peek_btn, on_peek, LV_EVENT_CLICKED, nullptr);
    ui.end_btn = button(bar, group, "End session", &font_fell_14);
    lv_obj_set_size(ui.end_btn, 130, 26);
    lv_obj_add_event_cb(ui.end_btn, on_end, LV_EVENT_CLICKED, nullptr);
    Navigator::instance().setHoldHandler(confirm_end);
    lv_group_focus_obj(ui.peek_btn);

    lv_subject_add_observer_obj(&state.focus_state, update_view, content, nullptr);
    lv_subject_add_observer_obj(&state.peek_active, update_view, content, nullptr);
    lv_subject_add_observer_obj(&state.always_show_timer, update_view, content, nullptr);
    lv_subject_add_observer_obj(&state.session_paused, update_view, content, nullptr);
}
