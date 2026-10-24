// 07 Workshop: live preview in the porthole and rows for Body (Bot / Cat / Dog),
// Face (Plain / Monocle) and Finish (4 swatches). The outlined row is where the knob is;
// the filled chip is the current choice. Turn moves between rows; press enters a row, where
// turning changes the choice (live) and pressing again sets it. Hold goes back.

#include <ui/screens.hpp>
#include <ui/navigator.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>
#include <ui/pet/pet_view.hpp>

#include "lvgl.h"

using namespace theme;

enum Option { OPT_BODY, OPT_FACE, OPT_FINISH, OPT_COUNT };

static const char *const BODY_NAMES[PET_BODY_COUNT] = {"Bot", "Cat", "Dog"};
static const char *const FACE_NAMES[PET_FACE_COUNT] = {"Plain", "Monocle"};
static const int OPTION_COUNTS[OPT_COUNT] = {PET_BODY_COUNT, PET_FACE_COUNT, PET_FINISH_COUNT};

static struct {
    lv_group_t *group;
    lv_obj_t *choices[OPT_COUNT]; // container of chips/swatches per row
    lv_obj_t *hint;
} ui;

static lv_subject_t *subject_for(int opt)
{
    AppState &state = AppState::instance();
    switch (opt) {
        case OPT_BODY: return &state.pet_body;
        case OPT_FACE: return &state.pet_face;
        default: return &state.pet_finish;
    }
}

// Filled chip / outlined swatch for the current choice
static void update_choices(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *choices = lv_observer_get_target_obj(observer);
    const int chosen = lv_subject_get_int(subject);
    const bool swatches = choices == ui.choices[OPT_FINISH];
    const uint32_t n = lv_obj_get_child_count(choices);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *item = lv_obj_get_child(choices, i);
        const bool on = (int)i == chosen;
        if (swatches) {
            lv_obj_set_style_outline_width(item, on ? 2 : 0, 0);
        } else {
            lv_obj_set_style_bg_opa(item, on ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
            lv_obj_t *text = lv_obj_get_child(item, 0);
            lv_obj_set_style_text_color(text, c(on ? PANEL : BRASS), 0);
            lv_obj_set_style_text_color(text, c(on ? PANEL : BRASS_HI), LV_STATE_USER_2);
        }
    }
}

static void update_title(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_label_set_text_fmt(lv_observer_get_target_obj(observer), "%s's Workshop", lv_subject_get_string(subject));
}

static void set_hint(bool editing)
{
    lv_label_set_text(ui.hint, editing ? "Turn to change · press to set" : "Turn to browse · press to pick · hold to leave");
}

static void on_row_click(lv_event_t *e)
{
    const bool editing = !lv_group_get_editing(ui.group);
    lv_group_set_editing(ui.group, editing);
    set_hint(editing);
}

static void on_row_key(lv_event_t *e)
{
    const uint32_t key = lv_event_get_key(e);
    if (key != LV_KEY_RIGHT && key != LV_KEY_LEFT) return;
    const int opt = (int)(intptr_t)lv_event_get_user_data(e);
    lv_subject_t *subject = subject_for(opt);
    const int n = OPTION_COUNTS[opt];
    const int next = (lv_subject_get_int(subject) + (key == LV_KEY_RIGHT ? 1 : n - 1)) % n;

    AppState &state = AppState::instance();
    int body = lv_subject_get_int(&state.pet_body);
    int face = lv_subject_get_int(&state.pet_face);
    int finish = lv_subject_get_int(&state.pet_finish);
    if (opt == OPT_BODY) body = next;
    else if (opt == OPT_FACE) face = next;
    else finish = next;
    state.setPet(body, face, finish);
}

static lv_obj_t *option_row(lv_obj_t *parent, int opt, const char *name)
{
    lv_obj_t *r = row(parent, LV_FLEX_ALIGN_START, 4);
    lv_obj_set_style_pad_all(r, 2, 0);
    lv_obj_t *ptr = pointer(r, &font_fell_12);
    lv_obj_t *name_label = label(r, name, &font_fell_12);
    lv_obj_set_width(name_label, 38);
    ui.choices[opt] = row(r, LV_FLEX_ALIGN_START, opt == OPT_FINISH ? 5 : 4);

    focusable_outline(r, ui.group, ptr);
    lv_obj_add_event_cb(r, on_row_click, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(r, on_row_key, LV_EVENT_KEY, (void *)(intptr_t)opt);
    return r;
}

void screen_workshop(lv_obj_t *parent, lv_group_t *group)
{
    AppState &state = AppState::instance();
    ui = {};
    ui.group = group;
    lv_obj_t *content = frame(parent);

    lv_obj_t *top = row(content);
    lv_obj_set_width(top, LV_PCT(100));
    lv_obj_t *title = label(top, "", &font_fell_16);
    lv_subject_add_observer_obj(&state.pet_name, update_title, title, nullptr);
    label(top, "press to save", &font_fell_12, MUTED);

    lv_obj_t *middle = row(content, LV_FLEX_ALIGN_START, 10);
    lv_obj_set_width(middle, LV_PCT(100));
    lv_obj_t *port = porthole(middle, 86);
    pet_view_create(port, 64, PET_MOOD_HAPPY);

    lv_obj_t *rows = column(middle, 8);
    lv_obj_t *first = option_row(rows, OPT_BODY, "Body");
    for (const char *name : BODY_NAMES) chip(ui.choices[OPT_BODY], name, &font_caslon_12);
    option_row(rows, OPT_FACE, "Face");
    for (const char *name : FACE_NAMES) chip(ui.choices[OPT_FACE], name, &font_caslon_12);
    option_row(rows, OPT_FINISH, "Finish");
    for (int i = 0; i < PET_FINISH_COUNT; i++) {
        lv_obj_t *swatch = box(ui.choices[OPT_FINISH]);
        lv_obj_set_size(swatch, 17, 17);
        lv_obj_set_style_bg_opa(swatch, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(swatch, c(pet_palettes[i][PET_SLOT_BODY]), 0);
        lv_obj_set_style_border_width(swatch, 1, 0);
        lv_obj_set_style_border_color(swatch, c(INK), 0);
        lv_obj_set_style_outline_color(swatch, c(BRASS_HI), 0);
        lv_obj_set_style_outline_pad(swatch, 1, 0);
    }
    for (int opt = 0; opt < OPT_COUNT; opt++)
        lv_subject_add_observer_obj(subject_for(opt), update_choices, ui.choices[opt], nullptr);

    ui.hint = label(content, "", &font_fell_12, MUTED);
    lv_obj_set_width(ui.hint, LV_PCT(100));
    lv_obj_set_style_text_align(ui.hint, LV_TEXT_ALIGN_CENTER, 0);
    set_hint(false);

    lv_group_focus_obj(first);
}
