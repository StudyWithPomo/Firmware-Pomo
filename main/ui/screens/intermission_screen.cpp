// 05 Intermission: verdigris frame, the break countdown and the wound-up pet. Touch is
// unlocked: tapping the pet, turning the knob, pressing or holding it winds the pet's key
// (a short wiggle). No hearts. When the break ends the next session starts on its own.

#include <ui/screens.hpp>
#include <ui/navigator.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>
#include <ui/pet/pet_view.hpp>
#include <app/session_core.hpp>

#include "lvgl.h"

using namespace theme;

static lv_obj_t *s_porthole;

static void wiggle_exec(void *obj, int32_t v)
{
    lv_obj_set_style_translate_x((lv_obj_t *)obj, v, 0);
}

static void wiggle_done(lv_anim_t *a)
{
    lv_obj_set_style_translate_x((lv_obj_t *)a->var, 0, 0);
}

// Translating instead of rotating: a rotated porthole would need a full-size layer buffer
static void wind()
{
    if (!s_porthole || lv_anim_get(s_porthole, wiggle_exec)) return;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_porthole);
    lv_anim_set_exec_cb(&a, wiggle_exec);
    lv_anim_set_values(&a, -4, 4);
    lv_anim_set_duration(&a, 70);
    lv_anim_set_reverse_duration(&a, 70);
    lv_anim_set_repeat_count(&a, 2);
    lv_anim_set_completed_cb(&a, wiggle_done);
    lv_anim_start(&a);
}

static void on_input(lv_event_t *e) { wind(); }

static void update_title(lv_observer_t *observer, lv_subject_t *subject)
{
    const bool long_rest = (Phase)lv_subject_get_int(subject) == Phase::LONG_REST;
    lv_label_set_text(lv_observer_get_target_obj(observer), long_rest ? "Long Rest" : "Intermission");
}

static void update_timer(lv_observer_t *observer, lv_subject_t *subject)
{
    char text[8];
    format_mmss(text, sizeof(text), lv_subject_get_int(subject));
    lv_label_set_text(lv_observer_get_target_obj(observer), text);
}

static void update_hint(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "Turn or hold to wind %s",
                          lv_subject_get_string(subject));
}

static void on_delete(lv_event_t *e) { s_porthole = nullptr; }

void screen_intermission(lv_obj_t *parent, lv_group_t *group)
{
    AppState &state = AppState::instance();
    lv_obj_t *content = frame(parent, VERDIGRIS);
    lv_obj_add_event_cb(content, on_delete, LV_EVENT_DELETE, nullptr);

    lv_obj_t *top = row(content);
    lv_obj_set_width(top, LV_PCT(100));
    lv_obj_t *title = label(top, "", &font_fell_16, VERDIGRIS_HI);
    lv_subject_add_observer_obj(&state.phase, update_title, title, nullptr);
    lv_obj_t *timer = label(top, "", &font_plex_22, NIXIE);
    lv_subject_add_observer_obj(&state.timer_remaining_s, update_timer, timer, nullptr);

    lv_obj_t *middle = row(content, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_width(middle, LV_PCT(100));
    s_porthole = porthole(middle, 98);
    lv_obj_t *pet = pet_view_create(s_porthole, 64, PET_MOOD_LOVE);
    lv_obj_add_flag(pet, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(pet, on_input, LV_EVENT_PRESSED, nullptr);

    lv_obj_t *hint_box = box(content);
    lv_obj_set_width(hint_box, LV_PCT(100));
    lv_obj_set_style_border_width(hint_box, 1, 0);
    lv_obj_set_style_border_color(hint_box, c(VERDIGRIS_HI), 0);
    lv_obj_set_style_pad_all(hint_box, 6, 0);
    lv_obj_t *hint = label(hint_box, "", &font_fell_14, VERDIGRIS_HI);
    lv_obj_set_width(hint, LV_PCT(100));
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_subject_add_observer_obj(&state.pet_name, update_hint, hint, nullptr);

    // Knob: turns arrive as keys (edit mode), presses as clicks, holds via the hold handler
    lv_group_add_obj(group, content);
    lv_obj_add_event_cb(content, on_input, LV_EVENT_KEY, nullptr);
    lv_obj_add_event_cb(content, on_input, LV_EVENT_CLICKED, nullptr);
    always_editing(group);
    Navigator::instance().setHoldHandler(wind);
}
