// Pet hub: the pet in its porthole (tap to pet) and rows for Status, Playground and
// Wardrobe. Turn to choose, press or tap to open; ◂ Back or hold goes back.

#include <ui/screens.hpp>
#include <ui/navigator.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>
#include <ui/pet/pet_view.hpp>

#include "lvgl.h"

using namespace theme;

static lv_obj_t *s_porthole;

static void update_level(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "Lv %d", (int)lv_subject_get_int(subject));
}

static void update_mood(lv_observer_t *observer, lv_subject_t *subject)
{
    pet_view_set_mood(lv_observer_get_target_obj(observer), (PetMood)lv_subject_get_int(subject));
}

static void on_pet_tap(lv_event_t *e)
{
    AppState::instance().petted();
    wiggle(s_porthole);
}

static void go(lv_event_t *e)
{
    Navigator::instance().navigate((ScreenID)(intptr_t)lv_event_get_user_data(e));
}

static void on_delete(lv_event_t *e) { s_porthole = nullptr; }

void screen_pet(lv_obj_t *parent, lv_group_t *group)
{
    AppState &state = AppState::instance();
    lv_obj_t *content = frame(parent);
    lv_obj_add_event_cb(content, on_delete, LV_EVENT_DELETE, nullptr);

    Header top = header(content, "");
    lv_subject_add_observer_obj(&state.pet_name, [](lv_observer_t *observer, lv_subject_t *subject) {
        lv_label_set_text(lv_observer_get_target_obj(observer), lv_subject_get_string(subject));
    }, top.title, nullptr);

    lv_obj_t *middle = row(content, LV_FLEX_ALIGN_START, 12);
    lv_obj_set_width(middle, LV_PCT(100));
    s_porthole = porthole(middle, 94);
    lv_obj_t *pet = pet_view_create(s_porthole, 64, PET_MOOD_HAPPY);
    lv_obj_add_flag(pet, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(pet, 15);
    lv_obj_add_event_cb(pet, on_pet_tap, LV_EVENT_PRESSED, nullptr);
    lv_subject_add_observer_obj(&state.pet_mood, update_mood, pet, nullptr);

    lv_obj_t *rows = column(middle);
    lv_obj_set_style_pad_all(rows, 4, 0); // room for the focus outline
    lv_obj_set_flex_grow(rows, 1);
    lv_obj_t *status = list_row(rows, group, "Status", "", &font_plex_12);
    lv_subject_add_observer_obj(&state.level, update_level, lv_obj_get_child(status, 1), nullptr);
    lv_obj_add_event_cb(status, go, LV_EVENT_CLICKED, (void *)(intptr_t)ScreenID::STATUS);
    lv_obj_t *playground = list_row(rows, group, "Playground", "soon", &font_caslon_12, MUTED);
    lv_obj_add_event_cb(playground, go, LV_EVENT_CLICKED, (void *)(intptr_t)ScreenID::PLAYGROUND);
    lv_obj_t *wardrobe = list_row(rows, group, "Wardrobe");
    lv_obj_add_event_cb(wardrobe, go, LV_EVENT_CLICKED, (void *)(intptr_t)ScreenID::WARDROBE);

    lv_obj_t *hint = label(content, "Tap the pet to give it a pat", &font_fell_12, MUTED);
    lv_obj_set_width(hint, LV_PCT(100));
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);

    lv_group_add_obj(group, top.back);
    lv_group_focus_obj(status);
}
