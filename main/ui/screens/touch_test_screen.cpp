// Touch calibration check. Shown instead of the normal UI when POMO_TOUCH_TEST is 1.
// Tap each corner marker: the dot should land on it. If it lands mirrored or on the
// wrong axis, fix touch_cfg.flags in hal/touch.c (not the calibration math).

#include <ui/screens.hpp>

#include "lvgl.h"

static lv_obj_t *dot;
static lv_obj_t *coords_label;

static void on_touch(lv_event_t *e)
{
    lv_indev_t *indev = lv_indev_active();
    if (indev == NULL) return;

    lv_point_t p;
    lv_indev_get_point(indev, &p);

    lv_obj_set_pos(dot, p.x - lv_obj_get_width(dot) / 2, p.y - lv_obj_get_height(dot) / 2);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text_fmt(coords_label, "x %d  y %d", (int)p.x, (int)p.y);
}

static void add_marker(lv_obj_t *parent, lv_align_t align, const char *text)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_remove_style_all(box);
    lv_obj_set_size(box, 16, 16);
    lv_obj_set_style_bg_color(box, lv_color_hex(0xC9A15A), 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_align(box, align, 0, 0);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(0xEFE3C8), 0);
    int dx = (align == LV_ALIGN_TOP_LEFT || align == LV_ALIGN_BOTTOM_LEFT) ? 20 : -20;
    int dy = (align == LV_ALIGN_TOP_LEFT || align == LV_ALIGN_TOP_RIGHT) ? 0 : -2;
    lv_obj_align(label, align, dx, dy);
}

void screen_touch_test(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x1E1510), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_remove_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    add_marker(parent, LV_ALIGN_TOP_LEFT, "TL");
    add_marker(parent, LV_ALIGN_TOP_RIGHT, "TR");
    add_marker(parent, LV_ALIGN_BOTTOM_LEFT, "BL");
    add_marker(parent, LV_ALIGN_BOTTOM_RIGHT, "BR");

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text_fmt(title, "Touch test  %dx%d",
                          (int)lv_display_get_horizontal_resolution(NULL),
                          (int)lv_display_get_vertical_resolution(NULL));
    lv_obj_set_style_text_color(title, lv_color_hex(0xC9A15A), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -14);

    coords_label = lv_label_create(parent);
    lv_label_set_text(coords_label, "Tap each corner");
    lv_obj_set_style_text_color(coords_label, lv_color_hex(0xEFE3C8), 0);
    lv_obj_align(coords_label, LV_ALIGN_CENTER, 0, 12);

    dot = lv_obj_create(parent);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, 10, 10);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0xFF5A00), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(dot, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_flag(parent, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(parent, on_touch, LV_EVENT_PRESSING, nullptr);
}
