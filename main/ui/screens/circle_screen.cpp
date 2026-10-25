// Study circle: the friends nearby, a row each, and studying together by invite. Tapping (or
// pressing) a free friend's row invites them, with this device's study and break lengths.
// While hosting, Start (once someone joined) and Cancel sit under the list; the session also
// starts by itself once every invite is answered. The badge is the multiplier the group earns.
// ◂ Back (the knob's last stop) or hold goes back.

#include <ui/screens.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>
#include <comms/circle.hpp>

#include "lvgl.h"

using namespace theme;

static struct {
    lv_group_t *group;
    lv_obj_t *back;
    lv_obj_t *list;
    lv_obj_t *bottom;
} ui;

static void update_badge(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *badge = lv_observer_get_target_obj(observer);
    lv_label_set_text_fmt(lv_obj_get_child(badge, 0), "%d× XP", (int)lv_subject_get_int(subject));
}

static bool invitable(const AppState::Friend &f, const AppState::CircleView &view)
{
    return view.can_invite && (f.status == FriendStatus::FREE || f.status == FriendStatus::DECLINED);
}

static const char *status_text(const AppState::Friend &f, const AppState::CircleView &view, uint32_t *color)
{
    *color = MUTED;
    switch (f.status) {
        case FriendStatus::FREE:
            if (!view.can_invite) return "Free";
            *color = BRASS_HI;
            return "Invite";
        case FriendStatus::BUSY:
            return f.studying ? "Studying" : "Busy";
        case FriendStatus::INVITED:
            *color = BRASS;
            return "Invited";
        case FriendStatus::DECLINED:
            *color = PAUSED;
            return "Declined";
        case FriendStatus::MEMBER:
            *color = VERDIGRIS_HI;
            if (view.role == GroupRole::HOSTING || view.role == GroupRole::JOINED) return "Joined";
            if (!f.studying) return "Resting";
            if (!f.focused) *color = PAUSED;
            return f.focused ? "Focused" : "Drifting";
    }
    return "";
}

static void on_row(lv_event_t *e)
{
    AppState &state = AppState::instance();
    const int i = (int)(intptr_t)lv_event_get_user_data(e);
    int count = 0;
    const AppState::Friend *friends = state.friends(&count);
    if (i >= count || !invitable(friends[i], state.circleView())) return;
    circle_invite(friends[i].mac, lv_subject_get_int(&state.study_min), lv_subject_get_int(&state.break_min),
                  lv_subject_get_int(&state.test_timing) != 0);
}

static void on_start(lv_event_t *e) { circle_start(); }
static void on_cancel(lv_event_t *e) { circle_cancel(); }
static void on_leave(lv_event_t *e) { circle_answer(false); }

static lv_obj_t *small_button(lv_obj_t *parent, const char *text, int width, lv_event_cb_t cb)
{
    lv_obj_t *btn = button(parent, ui.group, text, &font_fell_14);
    lv_obj_set_size(btn, width, 26);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
    return btn;
}

static void note(const char *text)
{
    lv_obj_t *l = label(ui.bottom, text, &font_caslon_12, PARCHMENT);
    lv_obj_set_width(l, LV_PCT(100));
    lv_label_set_long_mode(l, LV_LABEL_LONG_MODE_WRAP);
}

static void build_bottom(const AppState::CircleView &view, int count)
{
    switch (view.role) {
        case GroupRole::HOSTING: {
            lv_obj_t *r = row(ui.bottom, LV_FLEX_ALIGN_CENTER, 12);
            lv_obj_set_width(r, LV_PCT(100));
            lv_obj_set_style_pad_all(r, 4, 0); // room for the focus outline
            if (view.can_start) small_button(r, "Start", 100, on_start);
            else label(r, "Waiting for answers", &font_caslon_12, MUTED);
            small_button(r, "Cancel", 100, on_cancel);
            break;
        }
        case GroupRole::JOINED: {
            lv_obj_t *r = row(ui.bottom, LV_FLEX_ALIGN_CENTER, 12);
            lv_obj_set_width(r, LV_PCT(100));
            lv_obj_set_style_pad_all(r, 4, 0);
            lv_obj_t *l = label(r, "", &font_caslon_12, PARCHMENT);
            lv_label_set_text_fmt(l, "Waiting for %s", view.host[0] ? view.host : "the host");
            small_button(r, "Leave", 90, on_leave);
            break;
        }
        case GroupRole::IN_SET:
            note("Each friend studying with you adds 1× XP, up to 3×. Anyone drifting pauses it.");
            break;
        default:
            if (count == 0) note("No study buddies nearby yet.");
            else note("Tap a friend to invite them. Each one studying with you adds 1× XP, up to 3×.");
            break;
    }
}

// Which knob stop is focused, by position, so a rebuild can put it back
static int focused_index()
{
    lv_obj_t *focused = lv_group_get_focused(ui.group);
    const uint32_t n = lv_group_get_obj_count(ui.group);
    for (uint32_t i = 0; i < n; i++)
        if (lv_group_get_obj_by_index(ui.group, i) == focused) return (int)i;
    return 0;
}

static void rebuild(lv_observer_t *observer, lv_subject_t *subject)
{
    AppState &state = AppState::instance();
    const AppState::CircleView &view = state.circleView();
    const int was = focused_index();
    lv_obj_clean(ui.list);
    lv_obj_clean(ui.bottom);

    int count = 0;
    const AppState::Friend *friends = state.friends(&count);
    for (int i = 0; i < count; i++) {
        uint32_t color;
        const char *status = status_text(friends[i], view, &color);
        lv_obj_t *r = list_row(ui.list, ui.group, friends[i].name, status, &font_fell_14, color);
        lv_obj_set_height(r, 28);
        lv_obj_add_event_cb(r, on_row, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
    build_bottom(view, count);

    // ◂ Back stays the knob's last stop
    if (lv_obj_get_group(ui.back)) lv_group_remove_obj(ui.back);
    lv_group_add_obj(ui.group, ui.back);
    const uint32_t n = lv_group_get_obj_count(ui.group);
    lv_group_focus_obj(lv_group_get_obj_by_index(ui.group, (uint32_t)was < n ? (uint32_t)was : n - 1));
}

static void on_delete(lv_event_t *e) { ui = {}; }

void screen_circle(lv_obj_t *parent, lv_group_t *group)
{
    AppState &state = AppState::instance();
    ui = {};
    ui.group = group;
    lv_obj_t *content = frame(parent);
    lv_obj_add_event_cb(content, on_delete, LV_EVENT_DELETE, nullptr);

    Header top = header(content, "Study Circle");
    ui.back = top.back;
    lv_obj_t *badge = chip(top.row, "", &font_fell_14, true);
    lv_subject_add_observer_obj(&state.base_multiplier, update_badge, badge, nullptr);

    ui.list = column(content, 0);
    lv_obj_set_width(ui.list, LV_PCT(100));
    ui.bottom = column(content, 0);
    lv_obj_set_width(ui.bottom, LV_PCT(100));
    lv_subject_add_observer_obj(&state.circle_version, rebuild, content, nullptr);
}
