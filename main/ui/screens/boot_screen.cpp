// 00 Boot: "Study Buddy", day and time, the pet in its porthole, a greeting, and
// "Ready for a study session?" with Yes, wind it (-> Study) / Not yet (-> Home).

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <ui/screens.hpp>
#include <ui/navigator.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>
#include <ui/pet/pet_view.hpp>

#include "lvgl.h"

using namespace theme;

static bool local_time(struct tm *out)
{
    const long now = *(long *)lv_subject_get_pointer(&AppState::instance().time_subject);
    if (now < AppState::CLOCK_SET_AFTER) return false;
    time_t t = (time_t)now;
    localtime_r(&t, out);
    return true;
}

static void update_clock(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *label = lv_observer_get_target_obj(observer);
    struct tm tm;
    if (!local_time(&tm)) {
        lv_label_set_text(label, "--:--");
        return;
    }
    char day[8], clock[12];
    strftime(day, sizeof(day), "%a", &tm);
    const int hour12 = tm.tm_hour % 12 == 0 ? 12 : tm.tm_hour % 12;
    snprintf(clock, sizeof(clock), "%d:%02d %s", hour12, tm.tm_min, tm.tm_hour < 12 ? "AM" : "PM");
    lv_label_set_text_fmt(label, "%s · %s", day, clock);
}

static void update_greeting(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *bubble_box = lv_observer_get_target_obj(observer);
    lv_obj_t *text = lv_obj_get_child(bubble_box, 0);
    AppState &state = AppState::instance();

    const char *hello = "Hello";
    struct tm tm;
    if (local_time(&tm)) {
        if (tm.tm_hour >= 5 && tm.tm_hour < 12) hello = "Good morning";
        else if (tm.tm_hour >= 12 && tm.tm_hour < 17) hello = "Good afternoon";
        else hello = "Good evening";
    }
    const char *user = lv_subject_get_string(&state.user_name);
    lv_label_set_text_fmt(text, "%s, %s. %s missed you.", hello, user[0] ? user : "there",
                          lv_subject_get_string(&state.pet_name));
}

static void on_yes(lv_event_t *e) { Navigator::instance().navigate(ScreenID::STUDY); }
static void on_not_yet(lv_event_t *e) { Navigator::instance().reset(ScreenID::HOME); }

void screen_boot(lv_obj_t *parent, lv_group_t *group)
{
    AppState &state = AppState::instance();
    lv_obj_t *content = frame(parent);

    lv_obj_t *top = row(content);
    lv_obj_set_width(top, LV_PCT(100));
    label(top, "Study Buddy", &font_fell_14);
    lv_obj_t *clock = label(top, "", &font_plex_12);
    lv_subject_add_observer_obj(&state.time_subject, update_clock, clock, nullptr);

    lv_obj_t *middle = row(content, LV_FLEX_ALIGN_CENTER, 12);
    lv_obj_set_width(middle, LV_PCT(100));
    lv_obj_t *port = porthole(middle, 84);
    pettable(pet_view_create(port, 64, PET_MOOD_HAPPY), port);
    lv_obj_t *greeting = bubble(middle, "", 140);
    lv_subject_add_observer_obj(&state.user_name, update_greeting, greeting, nullptr);
    lv_subject_add_observer_obj(&state.pet_name, update_greeting, greeting, nullptr);
    lv_subject_add_observer_obj(&state.time_subject, update_greeting, greeting, nullptr);

    lv_obj_t *bottom = column(content, 7);
    lv_obj_set_width(bottom, LV_PCT(100));
    lv_obj_set_flex_align(bottom, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    label(bottom, "Ready for a study session?", &font_fell_14, PARCHMENT);

    lv_obj_t *buttons = row(bottom, LV_FLEX_ALIGN_START, 8);
    lv_obj_set_width(buttons, LV_PCT(100));
    lv_obj_t *yes = button(buttons, group, "Yes, wind it", &font_fell_14);
    lv_obj_set_height(yes, 32);
    lv_obj_set_flex_grow(yes, 7);
    lv_obj_add_event_cb(yes, on_yes, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *not_yet = button(buttons, group, "Not yet", &font_fell_14);
    lv_obj_set_height(not_yet, 32);
    lv_obj_set_flex_grow(not_yet, 5);
    lv_obj_add_event_cb(not_yet, on_not_yet, LV_EVENT_CLICKED, nullptr);

    lv_group_focus_obj(yes);
}
