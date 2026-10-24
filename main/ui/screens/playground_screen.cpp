// Playground: a placeholder until it's built (winding the key, feeding gears). ◂ Back or
// hold goes back.

#include <ui/screens.hpp>
#include <ui/navigator.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>
#include <ui/pet/pet_view.hpp>

#include "lvgl.h"

using namespace theme;

static void update_note(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer),
                          "Coming soon: wind %s's key with the knob, and feed it gears.", lv_subject_get_string(subject));
}

void screen_playground(lv_obj_t *parent, lv_group_t *group)
{
    AppState &state = AppState::instance();
    lv_obj_t *content = frame(parent);

    Header top = header(content, "Playground");

    lv_obj_t *middle = row(content, LV_FLEX_ALIGN_CENTER, 14);
    lv_obj_set_width(middle, LV_PCT(100));
    lv_obj_t *port = porthole(middle, 84);
    pet_view_create(port, 64, PET_MOOD_SLEEP);
    lv_obj_t *note = bubble(middle, "", 150);
    lv_subject_add_observer_obj(&state.pet_name, update_note, lv_obj_get_child(note, 0), nullptr);

    lv_obj_t *spacer = box(content); // keeps the pet centred under space-between
    lv_obj_set_height(spacer, 1);

    lv_group_add_obj(group, top.back);
}
