#include "ui_root.hpp"
#include "lvgl.h"

static lv_obj_t *content_area;

void ui_root_init()
{
    lv_obj_t *root = lv_screen_active();
    lv_obj_set_style_bg_color(root, lv_color_hex(0x000000), 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    content_area = lv_obj_create(root);
    lv_obj_set_size(content_area, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(content_area, 0, 0);
    lv_obj_set_style_border_width(content_area, 0, 0);
    lv_obj_set_style_radius(content_area, 0, 0);
    lv_obj_set_style_bg_opa(content_area, LV_OPA_TRANSP, 0);
}

lv_obj_t *ui_get_content_area()
{
    return content_area;
}
