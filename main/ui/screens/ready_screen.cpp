// Ready: the break ran out, and the device waits instead of starting the next session on its
// own. "Ready for session II of IV?" with Start next / Finish for now. Tapping the pet pets it.
// Holding the knob does nothing: there's nothing to go back to.

#include <ui/screens.hpp>
#include <ui/navigator.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>
#include <ui/pet/pet_view.hpp>
#include <app/session_core.hpp>

#include "lvgl.h"

using namespace theme;

static void update_question(lv_observer_t *observer, lv_subject_t *subject)
{
    // While READY, session_index is the session just finished
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "Ready for\nsession %s of IV?",
                          roman((int)lv_subject_get_int(subject) + 1));
}

static void update_mood(lv_observer_t *observer, lv_subject_t *subject)
{
    pet_view_set_mood(lv_observer_get_target_obj(observer), (PetMood)lv_subject_get_int(subject));
}

static void on_start_next(lv_event_t *e) { AppState::instance().startNextSession(); }
static void on_finish(lv_event_t *e) { AppState::instance().endSet(); }

void screen_ready(lv_obj_t *parent, lv_group_t *group)
{
    AppState &state = AppState::instance();
    lv_obj_t *content = frame(parent, VERDIGRIS);

    label(content, "Break's Over", &font_fell_16, VERDIGRIS_HI);

    lv_obj_t *middle = row(content, LV_FLEX_ALIGN_CENTER, 14);
    lv_obj_set_width(middle, LV_PCT(100));
    lv_obj_t *port = porthole(middle, 90);
    lv_obj_t *pet = pet_view_create(port, 64, PET_MOOD_HAPPY);
    pettable(pet, port);
    lv_subject_add_observer_obj(&state.pet_mood, update_mood, pet, nullptr);
    lv_obj_t *question = bubble(middle, "", 140);
    lv_subject_add_observer_obj(&state.session_index, update_question, lv_obj_get_child(question, 0), nullptr);

    lv_obj_t *buttons = row(content, LV_FLEX_ALIGN_CENTER, 12);
    lv_obj_set_width(buttons, LV_PCT(100));
    lv_obj_set_style_pad_all(buttons, 4, 0); // room for the focus outline
    lv_obj_t *next = button(buttons, group, "Start next", &font_fell_14);
    lv_obj_set_size(next, 120, 30);
    lv_obj_add_event_cb(next, on_start_next, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *finish = button(buttons, group, "Finish for now", &font_fell_14);
    lv_obj_set_size(finish, 132, 30);
    lv_obj_add_event_cb(finish, on_finish, LV_EVENT_CLICKED, nullptr);

    Navigator::instance().setHoldHandler(ignore_hold);
    lv_group_focus_obj(next);
}
