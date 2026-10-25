// 01 Home: pet name + level, a "Laptop" lamp while the dashboard is coupled, and XP; the pet
// in its porthole (tap to pet), a big Start and tiles for Study, Pet and Circle. The knob cycles Start -> Study -> Pet -> Circle.

#include <ui/screens.hpp>
#include <ui/navigator.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>
#include <ui/pet/pet_view.hpp>

#include "lvgl.h"

using namespace theme;

static void update_title(lv_observer_t *observer, lv_subject_t *subject)
{
    AppState &state = AppState::instance();
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "%s · Lv %d", lv_subject_get_string(&state.pet_name),
                          (int)lv_subject_get_int(&state.level));
}

static void update_laptop(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *lamp = lv_observer_get_target_obj(observer);
    if (lv_subject_get_int(subject)) lv_obj_remove_flag(lamp, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(lamp, LV_OBJ_FLAG_HIDDEN);
}

static void update_lengths(lv_observer_t *observer, lv_subject_t *subject)
{
    AppState &state = AppState::instance();
    lv_obj_t *label = lv_observer_get_target_obj(observer);
    if (lv_subject_get_int(&state.test_timing)) {
        lv_label_set_text(label, "test · 30 s");
        return;
    }
    lv_label_set_text_fmt(label, "%d / %d", (int)lv_subject_get_int(&state.study_min),
                          (int)lv_subject_get_int(&state.break_min));
}

static void update_today(lv_observer_t *observer, lv_subject_t *subject)
{
    AppState &state = AppState::instance();
    const int today = state.sessionsToday();
    lv_obj_t *label = lv_observer_get_target_obj(observer);
    if (state.today() == 0) lv_label_set_text(label, "Lengths, log");
    else lv_label_set_text_fmt(label, "%d today", today);
}

static void update_mood(lv_observer_t *observer, lv_subject_t *subject)
{
    pet_view_set_mood(lv_observer_get_target_obj(observer), (PetMood)lv_subject_get_int(subject));
}

static void update_mood_name(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_label_set_text(lv_observer_get_target_obj(observer), pet_mood_name(lv_subject_get_int(subject)));
}

static void update_near(lv_observer_t *observer, lv_subject_t *subject)
{
    int count = 0;
    AppState::instance().friends(&count);
    lv_obj_t *label = lv_observer_get_target_obj(observer);
    const int mult = lv_subject_get_int(&AppState::instance().base_multiplier);
    if (count == 0) lv_label_set_text(label, "Solo");
    else if (mult > 1) lv_label_set_text_fmt(label, "%d near · %d×", count, mult);
    else lv_label_set_text_fmt(label, "%d near", count);
}

static void on_start(lv_event_t *e) { AppState::instance().startSession(); }

static void go(lv_event_t *e)
{
    Navigator::instance().navigate((ScreenID)(intptr_t)lv_event_get_user_data(e));
}


// Outlined tile: name over a detail line. Returns the detail label.
static lv_obj_t *tile(lv_obj_t *parent, lv_group_t *group, const char *name, ScreenID to)
{
    lv_obj_t *t = column(parent, 1);
    lv_obj_set_flex_grow(t, 1);
    lv_obj_set_height(t, 42);
    lv_obj_set_flex_align(t, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_border_width(t, 1, 0);
    lv_obj_set_style_border_color(t, c(BRASS), 0);
    label(t, name, &font_fell_16);
    lv_obj_t *detail = label(t, "", &font_caslon_12, MUTED);
    focusable(t, group);
    lv_obj_set_ext_click_area(t, 3); // tiles sit 8 px apart
    lv_obj_add_event_cb(t, go, LV_EVENT_CLICKED, (void *)(intptr_t)to);
    return detail;
}

void screen_home(lv_obj_t *parent, lv_group_t *group)
{
    AppState &state = AppState::instance();
    lv_obj_t *content = frame(parent);

    // Top: "Mochi · Lv 3", the laptop lamp and the gears bar
    lv_obj_t *top = row(content);
    lv_obj_set_width(top, LV_PCT(100));
    lv_obj_t *title = label(top, "", &font_fell_14);
    lv_subject_add_observer_obj(&state.pet_name, update_title, title, nullptr);
    lv_subject_add_observer_obj(&state.level, update_title, title, nullptr);
    lv_obj_t *right = row(top, LV_FLEX_ALIGN_START, 12);
    lv_obj_t *laptop = row(right, LV_FLEX_ALIGN_START, 5);
    lv_obj_t *dot = box(laptop);
    lv_obj_set_size(dot, 6, 6);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(dot, c(VERDIGRIS_HI), 0);
    label(laptop, "Laptop", &font_caslon_12, MUTED);
    lv_subject_add_observer_obj(&state.link_connected, update_laptop, laptop, nullptr);
    gears_bar(right);

    // Middle: porthole + Start
    lv_obj_t *middle = row(content, LV_FLEX_ALIGN_START, 14);
    lv_obj_set_width(middle, LV_PCT(100));
    lv_obj_set_style_pad_all(middle, 4, 0); // room for the focus outline: rows clip their children
    lv_obj_t *port = porthole(middle, 94);
    lv_obj_t *pet = pet_view_create(port, 64, PET_MOOD_HAPPY);
    pettable(pet, port);
    lv_subject_add_observer_obj(&state.pet_mood, update_mood, pet, nullptr);

    lv_obj_t *start = column(middle, 2);
    lv_obj_set_flex_grow(start, 1);
    lv_obj_set_height(start, 70);
    lv_obj_set_flex_align(start, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_border_width(start, 1, 0);
    lv_obj_set_style_border_color(start, c(BRASS), 0);
    lv_obj_t *start_title = row(start, LV_FLEX_ALIGN_START);
    lv_obj_t *start_ptr = pointer(start_title, &font_fell_25);
    label(start_title, "Start", &font_fell_25);
    lv_obj_t *lengths = label(start, "", &font_plex_12);
    lv_subject_add_observer_obj(&state.study_min, update_lengths, lengths, nullptr);
    lv_subject_add_observer_obj(&state.break_min, update_lengths, lengths, nullptr);
    lv_subject_add_observer_obj(&state.test_timing, update_lengths, lengths, nullptr);
    focusable(start, group, start_ptr);
    lv_obj_add_event_cb(start, on_start, LV_EVENT_CLICKED, nullptr);

    // Bottom: Study, Pet, Circle
    lv_obj_t *tiles = row(content, LV_FLEX_ALIGN_START, 8);
    lv_obj_set_width(tiles, LV_PCT(100));
    lv_obj_set_style_pad_all(tiles, 4, 0);
    lv_obj_t *study = tile(tiles, group, "Study", ScreenID::STUDY);
    lv_subject_add_observer_obj(&state.log_version, update_today, study, nullptr);
    lv_subject_add_observer_obj(&state.time_subject, update_today, study, nullptr);
    lv_obj_t *pet_detail = tile(tiles, group, "Pet", ScreenID::PET);
    lv_subject_add_observer_obj(&state.pet_mood, update_mood_name, pet_detail, nullptr);
    lv_obj_t *circle = tile(tiles, group, "Circle", ScreenID::CIRCLE);
    lv_subject_add_observer_obj(&state.circle_version, update_near, circle, nullptr);
    lv_subject_add_observer_obj(&state.base_multiplier, update_near, circle, nullptr);

    lv_group_focus_obj(start);
}
