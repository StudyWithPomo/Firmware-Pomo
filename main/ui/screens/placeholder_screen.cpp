// Stand-in for screens that aren't built yet, and the knob navigation test:
// a list of every screen. Turn to move the highlight, press to open, hold 2 s to go back.

#include <stdint.h>

#include <ui/screens.hpp>
#include <ui/navigator.hpp>

#include "lvgl.h"

static const lv_color_t BRASS = lv_color_hex(0xC9A15A);
static const lv_color_t BRASS_HI = lv_color_hex(0xF4DDA0);
static const lv_color_t PANEL = lv_color_hex(0x1E1510);
static const lv_color_t MUTED = lv_color_hex(0xB8A27C);

static void on_click(lv_event_t *e)
{
    ScreenID id = (ScreenID)(intptr_t)lv_event_get_user_data(e);
    Navigator::instance().navigate(id);
}

// The focused row shows a pointer, as the design asks for
static void on_focus_change(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_current_target_obj(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    ScreenID id = (ScreenID)(intptr_t)lv_event_get_user_data(e);
    if (lv_event_get_code(e) == LV_EVENT_FOCUSED)
        lv_label_set_text_fmt(label, LV_SYMBOL_RIGHT " %s", screen_name(id));
    else
        lv_label_set_text_fmt(label, "   %s", screen_name(id));
}

static lv_obj_t *add_row(lv_obj_t *list, lv_group_t *group, ScreenID id)
{
    lv_obj_t *btn = lv_button_create(list);
    lv_obj_set_size(btn, LV_PCT(100), 32);
    lv_obj_set_style_radius(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x6B5233), 0);
    lv_obj_set_style_text_color(btn, BRASS, 0);
    lv_obj_set_style_pad_hor(btn, 8, 0);

    // Focused: filled brass with a highlight outline
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(btn, BRASS, LV_STATE_FOCUSED);
    lv_obj_set_style_text_color(btn, PANEL, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(btn, BRASS_HI, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(btn, 0, LV_STATE_FOCUSED);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text_fmt(label, "   %s", screen_name(id));
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

    void *ud = (void *)(intptr_t)id;
    lv_obj_add_event_cb(btn, on_click, LV_EVENT_CLICKED, ud);
    lv_obj_add_event_cb(btn, on_focus_change, LV_EVENT_FOCUSED, ud);
    lv_obj_add_event_cb(btn, on_focus_change, LV_EVENT_DEFOCUSED, ud);
    lv_group_add_obj(group, btn);
    return btn;
}

void screen_placeholder(lv_obj_t *parent, lv_group_t *group)
{
    Navigator &nav = Navigator::instance();
    const ScreenID self = nav.getCurrent();

    lv_obj_set_style_bg_color(parent, PANEL, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_remove_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 8, 0);
    lv_obj_set_style_pad_row(parent, 4, 0);

    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, LV_PCT(100), LV_SIZE_CONTENT);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, screen_name(self));
    lv_obj_set_style_text_color(title, BRASS, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *depth = lv_label_create(header);
    lv_label_set_text_fmt(depth, "placeholder - back %u", (unsigned)nav.historyDepth());
    lv_obj_set_style_text_color(depth, MUTED, 0);
    lv_obj_align(depth, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_t *list = lv_obj_create(parent);
    lv_obj_remove_style_all(list);
    lv_obj_set_width(list, LV_PCT(100));
    lv_obj_set_flex_grow(list, 1);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(list, 2, 0); // room for the focus outline

    for (int i = 0; i < (int)ScreenID::COUNT; i++) {
        ScreenID id = (ScreenID)i;
        if (id != self) add_row(list, group, id);
    }

    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "Turn: choose   Press: open   Hold: back");
    lv_obj_set_style_text_color(hint, MUTED, 0);
    lv_obj_set_width(hint, LV_PCT(100));
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
}
