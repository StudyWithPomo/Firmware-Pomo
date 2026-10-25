// Invite: a friend asked this device to study together. "Ibrahim invited you to a study
// session · 25 / 5" with Accept / Decline. After Accept it waits for the host to start
// ("Waiting for Ibrahim to start") with Leave; the session starting moves on to Focus.
// ◂ Back, Decline, Leave and a hold all say no and go back. AppState opens this screen when an
// invite arrives and closes it when the invite goes (cancelled, or run out).

#include <ui/screens.hpp>
#include <ui/navigator.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>
#include <ui/pet/pet_view.hpp>
#include <comms/circle.hpp>

#include "lvgl.h"

using namespace theme;

static struct {
    lv_group_t *group;
    lv_obj_t *back;
    lv_obj_t *text;
    lv_obj_t *buttons;
    GroupRole shown;
} ui;

static void decline_and_go()
{
    circle_answer(false);
    Navigator::instance().back();
}

static void on_accept(lv_event_t *e) { circle_answer(true); }
static void on_decline(lv_event_t *e) { decline_and_go(); }
static void on_back(lv_event_t *e) { circle_answer(false); } // header() goes back itself

static lv_obj_t *add_button(const char *text, int width, lv_event_cb_t cb)
{
    lv_obj_t *btn = button(ui.buttons, ui.group, text, &font_fell_14);
    lv_obj_set_size(btn, width, 30);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
    return btn;
}

static void update(lv_observer_t *observer, lv_subject_t *subject)
{
    const AppState::CircleView &view = AppState::instance().circleView();
    const char *host = view.host[0] ? view.host : "A friend";
    char lengths[24];
    if (view.test) lv_snprintf(lengths, sizeof(lengths), "Test timing");
    else lv_snprintf(lengths, sizeof(lengths), "%d / %d min", view.study, view.brk);

    if (view.role == GroupRole::JOINED)
        lv_label_set_text_fmt(ui.text, "Joined! Waiting for %s to start.\n%s", host, lengths);
    else
        lv_label_set_text_fmt(ui.text, "%s invited you to a study session.\n%s", host, lengths);

    // The buttons only change between invited and joined
    const GroupRole role = view.role == GroupRole::JOINED ? GroupRole::JOINED : GroupRole::INVITED;
    if (role == ui.shown) return;
    ui.shown = role;
    lv_obj_clean(ui.buttons);
    lv_obj_t *first;
    if (role == GroupRole::JOINED) {
        first = add_button("Leave", 110, on_decline);
    } else {
        first = add_button("Accept", 110, on_accept);
        add_button("Decline", 110, on_decline);
    }
    if (lv_obj_get_group(ui.back)) lv_group_remove_obj(ui.back);
    lv_group_add_obj(ui.group, ui.back);
    lv_group_focus_obj(first);
}

static void on_delete(lv_event_t *e) { ui = {}; }

void screen_invite(lv_obj_t *parent, lv_group_t *group)
{
    AppState &state = AppState::instance();
    ui = {};
    ui.group = group;
    ui.shown = GroupRole::NONE;
    lv_obj_t *content = frame(parent, VERDIGRIS);
    lv_obj_add_event_cb(content, on_delete, LV_EVENT_DELETE, nullptr);

    Header top = header(content, "Study Invite");
    ui.back = top.back;
    lv_obj_add_event_cb(ui.back, on_back, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *middle = row(content, LV_FLEX_ALIGN_CENTER, 14);
    lv_obj_set_width(middle, LV_PCT(100));
    lv_obj_t *port = porthole(middle, 84);
    pettable(pet_view_create(port, 56, PET_MOOD_HAPPY), port);
    lv_obj_t *bubble_box = bubble(middle, "", 160);
    ui.text = lv_obj_get_child(bubble_box, 0);

    ui.buttons = row(content, LV_FLEX_ALIGN_CENTER, 12);
    lv_obj_set_width(ui.buttons, LV_PCT(100));
    lv_obj_set_style_pad_all(ui.buttons, 4, 0); // room for the focus outline

    lv_subject_add_observer_obj(&state.circle_version, update, content, nullptr);
    Navigator::instance().setHoldHandler(decline_and_go);
}
