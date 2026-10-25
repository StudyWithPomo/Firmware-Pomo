// Status: the pet's level with an XP bar towards the next one, its mood, sessions completed
// and the next accessory it unlocks. Nothing to choose; ◂ Back or hold goes back.

#include <ui/screens.hpp>
#include <ui/navigator.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>
#include <ui/pet/pet_view.hpp>
#include <ui/pet/pet_data.h>
#include <app/session_core.hpp>

#include "lvgl.h"

using namespace theme;

static void update_title(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "%s's Status", lv_subject_get_string(subject));
}

static void update_level(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "Level %d", (int)lv_subject_get_int(subject));
}

static void update_bar(lv_observer_t *observer, lv_subject_t *subject)
{
    const int into = lv_subject_get_int(subject) % SessionCore::XP_PER_LEVEL;
    lv_obj_set_width(lv_observer_get_target_obj(observer), LV_PCT(into * 100 / SessionCore::XP_PER_LEVEL));
}

static void update_xp(lv_observer_t *observer, lv_subject_t *subject)
{
    const int xp = lv_subject_get_int(subject);
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "%d / %d XP to Lv %d", xp % SessionCore::XP_PER_LEVEL,
                          SessionCore::XP_PER_LEVEL, SessionCore::levelFor(xp) + 1);
}

static void update_mood_name(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_label_set_text(lv_observer_get_target_obj(observer), pet_mood_name(lv_subject_get_int(subject)));
}

static void update_sessions(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "%d", AppState::instance().log().sessions);
}

static void update_unlock(lv_observer_t *observer, lv_subject_t *subject)
{
    const int level = lv_subject_get_int(subject);
    lv_obj_t *label = lv_observer_get_target_obj(observer);
    if (level < PET_LEVEL_GOGGLES) lv_label_set_text_fmt(label, "Goggles at Lv %d", PET_LEVEL_GOGGLES);
    else if (level < PET_LEVEL_TIE) lv_label_set_text_fmt(label, "Bow tie at Lv %d", PET_LEVEL_TIE);
    else if (level < PET_LEVEL_HAT) lv_label_set_text_fmt(label, "Top hat at Lv %d", PET_LEVEL_HAT);
    else lv_label_set_text(label, "All unlocked");
}

// "just now", "5 min ago", "3 h ago", "yesterday", "4 days ago"; "never"; "–" without the time
static void update_fed(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *label = lv_observer_get_target_obj(observer);
    const long now = *(long *)lv_subject_get_pointer(&AppState::instance().time_subject);
    const long fed_at = AppState::instance().care().fed_at;
    if (now < AppState::CLOCK_SET_AFTER) { lv_label_set_text(label, "–"); return; }
    if (fed_at <= 0) { lv_label_set_text(label, "never"); return; }
    const long ago = now - fed_at;
    if (ago < 2 * 60) lv_label_set_text(label, "just now");
    else if (ago < 3600) lv_label_set_text_fmt(label, "%ld min ago", ago / 60);
    else if (ago < 24 * 3600) lv_label_set_text_fmt(label, "%ld h ago", ago / 3600);
    else if (ago < 48 * 3600) lv_label_set_text(label, "yesterday");
    else lv_label_set_text_fmt(label, "%ld days ago", ago / (24 * 3600));
}

static void update_mood(lv_observer_t *observer, lv_subject_t *subject)
{
    pet_view_set_mood(lv_observer_get_target_obj(observer), (PetMood)lv_subject_get_int(subject));
}

// "Name ........ value" in body type; returns the value label
static lv_obj_t *fact(lv_obj_t *parent, const char *name)
{
    lv_obj_t *r = row(parent);
    lv_obj_set_width(r, LV_PCT(100));
    lv_obj_set_style_border_width(r, 1, 0);
    lv_obj_set_style_border_side(r, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(r, c(TUBE_RIM), 0);
    lv_obj_set_style_pad_ver(r, 4, 0);
    label(r, name, &font_caslon_12, MUTED);
    return label(r, "", &font_caslon_12, PARCHMENT);
}

void screen_status(lv_obj_t *parent, lv_group_t *group)
{
    AppState &state = AppState::instance();
    lv_obj_t *content = frame(parent);

    Header top = header(content, "");
    lv_subject_add_observer_obj(&state.pet_name, update_title, top.title, nullptr);

    // Porthole + level and XP bar
    lv_obj_t *middle = row(content, LV_FLEX_ALIGN_START, 12);
    lv_obj_set_width(middle, LV_PCT(100));
    lv_obj_t *port = porthole(middle, 70, 2);
    lv_obj_t *pet = pet_view_create(port, 48, PET_MOOD_HAPPY);
    pettable(pet, port);
    lv_subject_add_observer_obj(&state.pet_mood, update_mood, pet, nullptr);

    lv_obj_t *progress = column(middle, 5);
    lv_obj_set_flex_grow(progress, 1);
    lv_obj_t *level = label(progress, "", &font_fell_25, BRASS_LIGHT);
    lv_subject_add_observer_obj(&state.level, update_level, level, nullptr);
    lv_obj_t *rail = box(progress);
    lv_obj_set_size(rail, LV_PCT(100), 9);
    lv_obj_set_style_border_width(rail, 1, 0);
    lv_obj_set_style_border_color(rail, c(BRASS), 0);
    lv_obj_set_style_pad_all(rail, 1, 0);
    lv_obj_t *fill = box(rail);
    lv_obj_set_height(fill, LV_PCT(100));
    lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(fill, c(BRASS), 0);
    lv_subject_add_observer_obj(&state.xp, update_bar, fill, nullptr);
    lv_obj_t *xp = label(progress, "", &font_plex_12, MUTED);
    lv_subject_add_observer_obj(&state.xp, update_xp, xp, nullptr);

    // Facts
    lv_obj_t *facts = column(content);
    lv_obj_set_width(facts, LV_PCT(100));
    lv_subject_add_observer_obj(&state.pet_mood, update_mood_name, fact(facts, "Mood"), nullptr);
    lv_subject_add_observer_obj(&state.log_version, update_sessions, fact(facts, "Sessions completed"), nullptr);
    lv_subject_add_observer_obj(&state.level, update_unlock, fact(facts, "Next unlock"), nullptr);
    lv_obj_t *fed = fact(facts, "Last fed");
    lv_subject_add_observer_obj(&state.time_subject, update_fed, fed, nullptr);
    lv_subject_add_observer_obj(&state.gears, update_fed, fed, nullptr);

    lv_group_add_obj(group, top.back);
}
