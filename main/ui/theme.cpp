#include "theme.hpp"

namespace theme {

// Focused knob stop: filled brass with a highlight outline
static lv_style_t s_focused;
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
static void set_descendants_state(lv_obj_t *obj, bool on)
{
    const uint32_t n = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *child = lv_obj_get_child(obj, i);
        if (on) lv_obj_add_state(child, LV_STATE_USER_1);
        else lv_obj_remove_state(child, LV_STATE_USER_1);
        set_descendants_state(child, on);
    }
}

static void on_focus_change(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_current_target_obj(e);
    lv_obj_t *ptr = (lv_obj_t *)lv_event_get_user_data(e);
    const bool focused = lv_event_get_code(e) == LV_EVENT_FOCUSED;
    set_descendants_state(obj, focused);
    if (ptr) {
        if (focused) lv_obj_remove_flag(ptr, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(ptr, LV_OBJ_FLAG_HIDDEN);
    }
}

void focusable(lv_obj_t *obj, lv_group_t *group, lv_obj_t *ptr)
{
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_style(obj, &s_focused, LV_STATE_FOCUSED);
    lv_obj_add_event_cb(obj, on_focus_change, LV_EVENT_FOCUSED, ptr);
    lv_obj_add_event_cb(obj, on_focus_change, LV_EVENT_DEFOCUSED, ptr);
    if (group) lv_group_add_obj(group, obj);
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

} // namespace theme
