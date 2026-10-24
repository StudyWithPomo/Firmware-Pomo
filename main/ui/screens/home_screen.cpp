// 01 Home: pet name + level, Menu chip, the pet in its porthole, the study circle card and
// a Begin button. The knob cycles Begin -> Circle -> Menu.

#include <ui/screens.hpp>
#include <ui/navigator.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>
#include <ui/pet/pet_view.hpp>

#include "lvgl.h"

using namespace theme;

static void update_title(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *label = lv_observer_get_target_obj(observer);
    AppState &state = AppState::instance();
    lv_label_set_text_fmt(label, "%s · Lv %d", lv_subject_get_string(&state.pet_name),
                          (int)lv_subject_get_int(&state.level));
}

static void update_multiplier(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "%d× XP", (int)lv_subject_get_int(subject));
}

static void update_friends(lv_observer_t *observer, lv_subject_t *subject)
{
    const char *names = lv_subject_get_string(subject);
    lv_label_set_text(lv_observer_get_target_obj(observer), names[0] ? names : "No one nearby");
}

static void update_begin_detail(lv_observer_t *observer, lv_subject_t *subject)
{
    AppState &state = AppState::instance();
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "%d / %d",
                          (int)lv_subject_get_int(&state.study_min), (int)lv_subject_get_int(&state.break_min));
}

static void on_begin(lv_event_t *e) { AppState::instance().startSession(); }

static void go(lv_event_t *e)
{
    Navigator::instance().navigate((ScreenID)(intptr_t)lv_event_get_user_data(e));
}

void screen_home(lv_obj_t *parent, lv_group_t *group)
{
    AppState &state = AppState::instance();
    lv_obj_t *content = frame(parent);

    // Top: "Mochi · Lv 3" and the Menu chip
    lv_obj_t *top = row(content);
    lv_obj_set_width(top, LV_PCT(100));
    lv_obj_t *title = label(top, "", &font_fell_14);
    lv_subject_add_observer_obj(&state.pet_name, update_title, title, nullptr);
    lv_subject_add_observer_obj(&state.level, update_title, title, nullptr);

    lv_obj_t *menu = chip(top, "Menu", &font_fell_12);

    // Middle: porthole + study circle card
    lv_obj_t *middle = row(content, LV_FLEX_ALIGN_CENTER, 16);
    lv_obj_set_width(middle, LV_PCT(100));
    lv_obj_t *port = porthole(middle, 94);
    pet_view_create(port, 64, PET_MOOD_HAPPY);

    lv_obj_t *card = column(middle, 2);
    lv_obj_set_width(card, 100);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, c(BRASS), 0);
    lv_obj_set_style_pad_ver(card, 6, 0);
    lv_obj_set_style_pad_hor(card, 8, 0);
    lv_obj_t *card_title = row(card, LV_FLEX_ALIGN_START);
    lv_obj_t *card_ptr = pointer(card_title, &font_fell_12);
    label(card_title, "Study circle", &font_fell_12);
    lv_obj_t *mult = label(card, "", &font_fell_25, BRASS_LIGHT);
    lv_subject_add_observer_obj(&state.multiplier, update_multiplier, mult, nullptr);
    lv_obj_t *friends = label(card, "", &font_caslon_12, PARCHMENT);
    lv_obj_set_width(friends, LV_PCT(100));
    lv_label_set_long_mode(friends, LV_LABEL_LONG_MODE_DOTS);
    lv_subject_add_observer_obj(&state.circle_names, update_friends, friends, nullptr);

    // Bottom: Begin 25 / 5, focused first
    lv_obj_t *begin = button(content, group, "Begin", &font_fell_16, "", &font_plex_12);
    lv_obj_set_size(begin, LV_PCT(100), 38);
    lv_obj_t *begin_detail = lv_obj_get_child(begin, 1);
    lv_subject_add_observer_obj(&state.study_min, update_begin_detail, begin_detail, nullptr);
    lv_subject_add_observer_obj(&state.break_min, update_begin_detail, begin_detail, nullptr);
    lv_obj_add_event_cb(begin, on_begin, LV_EVENT_CLICKED, nullptr);

    // Knob order: Begin -> Circle -> Menu
    focusable(card, group, card_ptr);
    lv_obj_add_event_cb(card, go, LV_EVENT_CLICKED, (void *)(intptr_t)ScreenID::CIRCLE);
    lv_obj_t *menu_ptr = pointer(menu, &font_fell_12);
    lv_obj_move_to_index(menu_ptr, 0);
    lv_obj_set_flex_flow(menu, LV_FLEX_FLOW_ROW);
    focusable(menu, group, menu_ptr);
    lv_obj_add_event_cb(menu, go, LV_EVENT_CLICKED, (void *)(intptr_t)ScreenID::MENU);

    lv_group_focus_obj(begin);
}
