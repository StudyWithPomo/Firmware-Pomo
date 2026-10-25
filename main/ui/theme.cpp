#include "theme.hpp"

#include <stdio.h>
#include <string.h>

#include <ui/app_state.hpp>
#include <ui/navigator.hpp>
#include <ui/pet/pet_data.h>
#include <hal/buzzer.h>

namespace theme {

// Focused knob stop: filled brass with a highlight outline
static lv_style_t s_focused;
// Outline-only knob stop (Workshop rows)
static lv_style_t s_focused_outline;
// Either kind while grabbed (the group is editing)
static lv_style_t s_grabbed;
static lv_style_t s_tube;
static bool s_ready = false;

void init()
{
    if (s_ready) return;

    lv_style_init(&s_focused);
    lv_style_set_bg_opa(&s_focused, LV_OPA_COVER);
    lv_style_set_bg_color(&s_focused, c(BRASS));
    lv_style_set_border_color(&s_focused, c(BRASS));
    lv_style_set_outline_width(&s_focused, 2);
    lv_style_set_outline_color(&s_focused, c(BRASS_HI));
    lv_style_set_outline_pad(&s_focused, 2);

    lv_style_init(&s_focused_outline);
    lv_style_set_outline_width(&s_focused_outline, 2);
    lv_style_set_outline_color(&s_focused_outline, c(BRASS_HI));
    lv_style_set_outline_pad(&s_focused_outline, 3);

    lv_style_init(&s_grabbed);
    lv_style_set_outline_color(&s_grabbed, c(VERDIGRIS_HI));

    // Unlit nixie glass: dark gradient, brass-brown rim
    lv_style_init(&s_tube);
    lv_style_set_bg_opa(&s_tube, LV_OPA_COVER);
    lv_style_set_bg_color(&s_tube, c(TUBE_TOP));
    lv_style_set_bg_grad_color(&s_tube, c(TUBE_BOTTOM));
    lv_style_set_bg_grad_dir(&s_tube, LV_GRAD_DIR_VER);
    lv_style_set_border_width(&s_tube, 1);
    lv_style_set_border_color(&s_tube, c(TUBE_RIM));

    s_ready = true;
}

lv_obj_t *box(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    return obj;
}

lv_obj_t *row(lv_obj_t *parent, lv_flex_align_t main, int gap)
{
    lv_obj_t *obj = box(parent);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj, main, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(obj, gap, 0);
    return obj;
}

lv_obj_t *column(lv_obj_t *parent, int gap)
{
    lv_obj_t *obj = box(parent);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(obj, gap, 0);
    return obj;
}

lv_obj_t *frame(lv_obj_t *parent, uint32_t border_color)
{
    lv_obj_remove_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = box(parent);
    lv_obj_set_size(panel, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(panel, c(PANEL), 0);
    lv_obj_set_style_pad_all(panel, 7, 0);

    // Double rule: two 1 px lines with a 1 px gap
    lv_obj_t *outer = box(panel);
    lv_obj_set_size(outer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_border_width(outer, 1, 0);
    lv_obj_set_style_border_color(outer, c(border_color), 0);
    lv_obj_set_style_pad_all(outer, 1, 0);

    lv_obj_t *inner = box(outer);
    lv_obj_set_size(inner, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_border_width(inner, 1, 0);
    lv_obj_set_style_border_color(inner, c(border_color), 0);
    lv_obj_set_style_pad_top(inner, 7, 0);
    lv_obj_set_style_pad_bottom(inner, 9, 0);
    lv_obj_set_style_pad_hor(inner, 9, 0);
    lv_obj_set_flex_flow(inner, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(inner, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    return inner;
}

lv_obj_t *label(lv_obj_t *parent, const char *text, const lv_font_t *font, uint32_t color)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_label_set_text(obj, text);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, c(color), 0);
    // Inside a focused stop (LV_STATE_USER_1, see focusable()) text turns dark on the brass
    lv_obj_set_style_text_color(obj, c(PANEL), LV_STATE_USER_1);
    // Inside an outline-focused row (LV_STATE_USER_2) text is highlighted instead
    lv_obj_set_style_text_color(obj, c(BRASS_HI), LV_STATE_USER_2);
    return obj;
}

lv_obj_t *porthole(lv_obj_t *parent, int size, int rim)
{
    lv_obj_t *obj = box(parent);
    lv_obj_set_size(obj, size, size);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(obj, c(PORTHOLE), 0);
    lv_obj_set_style_border_width(obj, rim, 0);
    lv_obj_set_style_border_color(obj, c(BRASS), 0);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    return obj;
}

lv_obj_t *bubble(lv_obj_t *parent, const char *text, int width)
{
    lv_obj_t *obj = box(parent);
    lv_obj_set_width(obj, width);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(obj, c(PARCHMENT), 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, c(BRASS), 0);
    lv_obj_set_style_pad_ver(obj, 7, 0);
    lv_obj_set_style_pad_hor(obj, 8, 0);

    lv_obj_t *text_label = label(obj, text, &font_caslon_12, INK);
    lv_obj_set_width(text_label, LV_PCT(100));
    lv_label_set_long_mode(text_label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_line_space(text_label, 2, 0);
    return obj;
}

lv_obj_t *chip(lv_obj_t *parent, const char *text, const lv_font_t *font, bool filled)
{
    lv_obj_t *obj = box(parent);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, c(BRASS), 0);
    lv_obj_set_style_pad_ver(obj, 1, 0);
    lv_obj_set_style_pad_hor(obj, 7, 0);
    lv_obj_t *text_label = label(obj, text, font, filled ? PANEL : BRASS);
    if (filled) {
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(obj, c(BRASS), 0);
    }
    (void)text_label;
    return obj;
}

// Copy the focused state onto every descendant so labels and pointers can follow it
static void set_descendants_state(lv_obj_t *obj, lv_state_t state, bool on)
{
    const uint32_t n = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *child = lv_obj_get_child(obj, i);
        if (on) lv_obj_add_state(child, state);
        else lv_obj_remove_state(child, state);
        set_descendants_state(child, state, on);
    }
}

static void apply_focus(lv_event_t *e, lv_state_t state)
{
    lv_obj_t *obj = lv_event_get_current_target_obj(e);
    lv_obj_t *ptr = (lv_obj_t *)lv_event_get_user_data(e);
    const bool focused = lv_event_get_code(e) == LV_EVENT_FOCUSED;
    set_descendants_state(obj, state, focused);
    if (ptr) {
        if (focused) lv_obj_remove_flag(ptr, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(ptr, LV_OBJ_FLAG_HIDDEN);
    }
}

static void on_focus_change(lv_event_t *e) { apply_focus(e, LV_STATE_USER_1); }
static void on_outline_focus_change(lv_event_t *e) { apply_focus(e, LV_STATE_USER_2); }
// Every button and row ticks when pressed, by touch or the knob
static void on_press_tick(lv_event_t *e) { buzzer_play(BUZZ_TAP); }

void focusable(lv_obj_t *obj, lv_group_t *group, lv_obj_t *ptr)
{
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(obj, TAP_MARGIN);
    lv_obj_add_style(obj, &s_focused, LV_STATE_FOCUSED);
    lv_obj_add_style(obj, &s_grabbed, LV_STATE_FOCUSED | LV_STATE_EDITED);
    lv_obj_add_event_cb(obj, on_focus_change, LV_EVENT_FOCUSED, ptr);
    lv_obj_add_event_cb(obj, on_focus_change, LV_EVENT_DEFOCUSED, ptr);
    lv_obj_add_event_cb(obj, on_press_tick, LV_EVENT_PRESSED, nullptr);
    if (group) lv_group_add_obj(group, obj);
}

void focusable_outline(lv_obj_t *obj, lv_group_t *group, lv_obj_t *ptr)
{
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_style(obj, &s_focused_outline, LV_STATE_FOCUSED);
    lv_obj_add_style(obj, &s_grabbed, LV_STATE_FOCUSED | LV_STATE_EDITED);
    lv_obj_add_event_cb(obj, on_outline_focus_change, LV_EVENT_FOCUSED, ptr);
    lv_obj_add_event_cb(obj, on_outline_focus_change, LV_EVENT_DEFOCUSED, ptr);
    lv_obj_add_event_cb(obj, on_press_tick, LV_EVENT_PRESSED, nullptr);
    if (group) lv_group_add_obj(group, obj);
}

bool from_knob()
{
    lv_indev_t *indev = lv_indev_active();
    return !indev || lv_indev_get_type(indev) == LV_INDEV_TYPE_ENCODER;
}

void always_editing(lv_group_t *group)
{
    // lv_group_focus_obj() clears editing before calling the focus callback
    lv_group_set_focus_cb(group, [](lv_group_t *g) { lv_group_set_editing(g, true); });
    lv_group_set_editing(group, true);
}

lv_obj_t *pointer(lv_obj_t *parent, const lv_font_t *font)
{
    lv_obj_t *obj = label(parent, UI_POINTER " ", font);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    return obj;
}

lv_obj_t *button(lv_obj_t *parent, lv_group_t *group, const char *text, const lv_font_t *font,
                 const char *detail, const lv_font_t *detail_font)
{
    lv_obj_t *btn = box(parent);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, c(BRASS), 0);
    lv_obj_set_style_pad_hor(btn, 12, 0);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn, detail ? LV_FLEX_ALIGN_SPACE_BETWEEN : LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *left = row(btn, LV_FLEX_ALIGN_START);
    lv_obj_t *ptr = pointer(left, font);
    label(left, text, font);
    if (detail) label(btn, detail, detail_font ? detail_font : font);

    focusable(btn, group, ptr);
    return btn;
}

lv_obj_t *list_row(lv_obj_t *parent, lv_group_t *group, const char *text,
                   const char *detail, const lv_font_t *detail_font, uint32_t detail_color)
{
    lv_obj_t *r = box(parent);
    lv_obj_set_size(r, LV_PCT(100), 32);
    lv_obj_set_style_border_width(r, 1, 0);
    lv_obj_set_style_border_side(r, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(r, c(TUBE_RIM), 0);
    lv_obj_set_style_pad_hor(r, 9, 0);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *left = row(r, LV_FLEX_ALIGN_START);
    lv_obj_t *ptr = pointer(left, &font_fell_16);
    label(left, text, &font_fell_16);
    if (detail) label(r, detail, detail_font ? detail_font : &font_plex_12, detail_color);

    focusable(r, group, ptr);
    lv_obj_set_ext_click_area(r, 0); // rows are tall enough, and stacked edge to edge
    return r;
}

static void on_back(lv_event_t *e) { Navigator::instance().back(); }
// Runs after focusable()'s tick and replaces it
static void on_back_press(lv_event_t *e) { buzzer_play(BUZZ_BACK); }

Header header(lv_obj_t *content, const char *title, const lv_font_t *font)
{
    Header h;
    h.row = row(content);
    lv_obj_set_width(h.row, LV_PCT(100));
    lv_obj_t *left = row(h.row, LV_FLEX_ALIGN_START, 10);
    h.back = chip(left, UI_BACK " Back", &font_fell_12);
    lv_obj_set_style_pad_ver(h.back, 2, 0);
    focusable(h.back, nullptr);
    lv_obj_add_event_cb(h.back, on_back, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(h.back, on_back_press, LV_EVENT_PRESSED, nullptr);
    h.title = label(left, title, font);
    return h;
}

static void on_step(lv_event_t *e)
{
    lv_obj_t *target = (lv_obj_t *)lv_event_get_user_data(e);
    uint32_t key = (uint32_t)(uintptr_t)lv_obj_get_user_data(lv_event_get_current_target_obj(e));
    lv_group_t *group = lv_obj_get_group(target);
    buzzer_play(BUZZ_STEP);
    if (lv_event_get_code(e) == LV_EVENT_PRESSED && group && lv_group_get_focused(group) != target)
        lv_group_focus_obj(target);
    lv_obj_send_event(target, LV_EVENT_KEY, &key);
}

static lv_obj_t *step_button(lv_obj_t *parent, lv_obj_t *target, const char *text, uint32_t key)
{
    lv_obj_t *btn = box(parent);
    lv_obj_set_size(btn, 26, 22);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, c(BRASS), 0);
    lv_obj_set_style_border_color(btn, c(PANEL), LV_STATE_USER_1); // inside a focused row
    lv_obj_set_style_bg_color(btn, c(TUBE_RIM), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_PRESSED);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    label(btn, text, &font_fell_16);

    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_set_ext_click_area(btn, 3); // half the gap between the two
    lv_obj_set_user_data(btn, (void *)(uintptr_t)key);
    lv_obj_add_event_cb(btn, on_step, LV_EVENT_PRESSED, target);
    lv_obj_add_event_cb(btn, on_step, LV_EVENT_LONG_PRESSED_REPEAT, target);
    return btn;
}

lv_obj_t *step_buttons(lv_obj_t *parent, lv_obj_t *target)
{
    lv_obj_t *r = row(parent, LV_FLEX_ALIGN_START, 6);
    step_button(r, target, UI_MINUS, LV_KEY_LEFT);
    step_button(r, target, "+", LV_KEY_RIGHT);
    return r;
}

// ---- Petting hearts ----

static const int HEART_W = 7, HEART_H = 6, HEART_SCALE = 2;
static const int HEART_RISE = 38;       // px
static const uint32_t HEART_MS = 900;
// .XX.XX.
// XXXXXXX
// XXXXXXX
// .XXXXX.
// ..XXX..
// ...X...
static const PetRect HEART_RECTS[] = { {1, 0, 2, 1}, {4, 0, 2, 1}, {0, 1, 7, 2}, {1, 3, 5, 1}, {2, 4, 3, 1}, {3, 5, 1, 1} };

// The heart's opacity is its user data, so fading needs no extra layer
static void heart_draw(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_current_target_obj(e);
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = c(COPPER);
    dsc.bg_opa = (lv_opa_t)(uintptr_t)lv_obj_get_user_data(obj);
    dsc.radius = 0;
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);
    for (const PetRect &r : HEART_RECTS) {
        lv_area_t a;
        a.x1 = coords.x1 + r.x * HEART_SCALE;
        a.y1 = coords.y1 + r.y * HEART_SCALE;
        a.x2 = a.x1 + r.w * HEART_SCALE - 1;
        a.y2 = a.y1 + r.h * HEART_SCALE - 1;
        lv_draw_rect(lv_event_get_layer(e), &dsc, &a);
    }
}

// v runs 0..1000: rise all the way, fade over the second half
static void heart_exec(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_translate_y(obj, -HEART_RISE * v / 1000, 0);
    const int opa = v < 500 ? LV_OPA_COVER : LV_OPA_COVER * (1000 - v) / 500;
    lv_obj_set_user_data(obj, (void *)(uintptr_t)opa);
    lv_obj_invalidate(obj);
}

static void heart_done(lv_anim_t *a) { lv_obj_delete_async((lv_obj_t *)a->var); }

static void hop_exec(void *obj, int32_t v) { lv_obj_set_style_translate_y((lv_obj_t *)obj, v, 0); }

void pet_hearts(lv_obj_t *porthole)
{
    if (!porthole) return;
    buzzer_play(BUZZ_PET);
    lv_area_t from;
    lv_obj_get_coords(porthole, &from);
    const int32_t cx = (from.x1 + from.x2) / 2;

    static const int OFFSET_X[3] = {-22, 2, 18};
    static const int OFFSET_Y[3] = {6, 0, 10};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *heart = lv_obj_create(lv_layer_top());
        lv_obj_remove_style_all(heart);
        lv_obj_remove_flag(heart, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_size(heart, HEART_W * HEART_SCALE, HEART_H * HEART_SCALE);
        lv_obj_set_pos(heart, cx + OFFSET_X[i] - HEART_W, from.y1 + OFFSET_Y[i]);
        lv_obj_set_user_data(heart, (void *)(uintptr_t)LV_OPA_COVER);
        lv_obj_add_event_cb(heart, heart_draw, LV_EVENT_DRAW_MAIN, nullptr);

        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, heart);
        lv_anim_set_exec_cb(&a, heart_exec);
        lv_anim_set_values(&a, 0, 1000);
        lv_anim_set_duration(&a, HEART_MS);
        lv_anim_set_delay(&a, i * 120);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_completed_cb(&a, heart_done);
        lv_anim_start(&a);
    }

    if (!lv_anim_get(porthole, hop_exec)) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, porthole);
        lv_anim_set_exec_cb(&a, hop_exec);
        lv_anim_set_values(&a, 0, -5);
        lv_anim_set_duration(&a, 110);
        lv_anim_set_reverse_duration(&a, 140);
        lv_anim_start(&a);
    }
}

static lv_obj_t *s_notice;
static lv_timer_t *s_notice_timer;

static void close_notice()
{
    if (s_notice) lv_obj_delete_async(s_notice);
    s_notice = nullptr;
    if (s_notice_timer) lv_timer_pause(s_notice_timer);
}

void notice(const char *title, const char *detail, uint32_t ms)
{
    close_notice();
    s_notice = column(lv_layer_top(), 2);
    lv_obj_set_width(s_notice, 264);
    lv_obj_align(s_notice, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_flex_align(s_notice, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(s_notice, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s_notice, c(PANEL), 0);
    lv_obj_set_style_border_width(s_notice, 2, 0);
    lv_obj_set_style_border_color(s_notice, c(BRASS_HI), 0);
    lv_obj_set_style_pad_ver(s_notice, 7, 0);
    lv_obj_set_style_pad_hor(s_notice, 10, 0);
    label(s_notice, title, &font_fell_16, BRASS_HI);
    if (detail && detail[0]) label(s_notice, detail, &font_caslon_12, PARCHMENT);
    lv_obj_add_flag(s_notice, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_notice, [](lv_event_t *) { close_notice(); }, LV_EVENT_CLICKED, nullptr);

    if (!s_notice_timer) s_notice_timer = lv_timer_create([](lv_timer_t *) { close_notice(); }, ms, nullptr);
    lv_timer_set_period(s_notice_timer, ms);
    lv_timer_reset(s_notice_timer);
    lv_timer_resume(s_notice_timer);
}

void ignore_hold() {}

void format_mmss(char *buf, size_t size, int seconds)
{
    if (seconds < 0) seconds = 0;
    int m = seconds / 60;
    if (m > 99) m = 99;
    snprintf(buf, size, "%02d:%02d", m, seconds % 60);
}

const char *roman(int n)
{
    static const char *const NUMERALS[] = {"I", "II", "III", "IV"};
    return n >= 1 && n <= 4 ? NUMERALS[n - 1] : "?";
}

lv_obj_t *nixie_row(lv_obj_t *parent, const NixieSpec &spec, bool lit)
{
    lv_obj_t *r = row(parent, LV_FLEX_ALIGN_CENTER, spec.gap);
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    for (int i = 0; i < 5; i++) {
        if (i == 2) {
            lv_obj_t *colon = label(r, ":", spec.font, lit ? NIXIE : TUBE_RIM);
            lv_obj_set_width(colon, spec.colon_w);
            lv_obj_set_style_text_align(colon, LV_TEXT_ALIGN_CENTER, 0);
            continue;
        }
        lv_obj_t *tube = box(r);
        lv_obj_set_size(tube, spec.tube_w, spec.tube_h);
        lv_obj_add_style(tube, &s_tube, 0);
        lv_obj_set_style_radius(tube, spec.radius, 0);
        lv_obj_t *digit = label(tube, lit ? "0" : "", spec.font, NIXIE);
        lv_obj_center(digit);
    }
    return r;
}

void nixie_set(lv_obj_t *r, int seconds)
{
    char text[8];
    format_mmss(text, sizeof(text), seconds);
    static const int tube_char[5] = {0, 1, -1, 3, 4};
    for (int i = 0; i < 5; i++) {
        if (tube_char[i] < 0) continue;
        lv_obj_t *tube = lv_obj_get_child(r, i);
        lv_obj_t *digit = tube ? lv_obj_get_child(tube, 0) : nullptr;
        if (!digit) continue;
        const char ch[2] = {text[tube_char[i]], '\0'};
        if (strcmp(lv_label_get_text(digit), ch) != 0) lv_label_set_text(digit, ch);
    }
}

lv_obj_t *lamp(lv_obj_t *parent, const char *name, bool focused)
{
    lv_obj_t *r = row(parent, LV_FLEX_ALIGN_START, 5);
    lv_obj_t *dot = box(r);
    lv_obj_set_size(dot, 6, 6);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(dot, c(focused ? VERDIGRIS_HI : COPPER), 0);
    lv_obj_t *text = label(r, "", &font_caslon_12, BRASS);
    lv_label_set_text_fmt(text, "%s · %s", name, focused ? "focused" : "drifting");
    return r;
}

void fill_lamps(lv_obj_t *container, const char *empty_text)
{
    lv_obj_clean(container);
    int count = 0;
    const AppState::Friend *friends = AppState::instance().friends(&count);
    int shown = 0;
    for (int i = 0; i < count; i++) {
        if (friends[i].status != FriendStatus::MEMBER) continue;
        lamp(container, friends[i].name, friends[i].focused);
        shown++;
    }
    if (shown == 0 && empty_text) label(container, empty_text, &font_caslon_12, MUTED);
}

} // namespace theme
