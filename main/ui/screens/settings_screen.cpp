#include <ui/screens.hpp>

void screen_settings(lv_obj_t* parent) {
    SystemAPI system = SystemAPI::instance();

    lv_obj_t *bg = lv_obj_create(parent);
    lv_obj_set_size(bg, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), 0);
    lv_obj_set_style_pad_all(bg, 0, 0);
    lv_obj_set_style_border_width(bg, 0, 0);
    lv_obj_set_style_radius(bg, 0, 0);
}