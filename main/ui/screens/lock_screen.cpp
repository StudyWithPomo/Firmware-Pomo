#include <ui/screen.hpp>
#include "lock_screen.hpp"
#include "lvgl.h"

void LockScreen::render() {
    lv_obj_t *bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bg, lv_pct(100), lv_pct(100));
    lv_obj_set_style_border_width(bg, 0, 0);
    lv_obj_set_style_radius(bg, 0, 0);
    lv_obj_set_style_bg_color(bg, lv_color_hex(0x0), 0);

    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Device Locked");
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *top_bar = lv_obj_create(lv_scr_act());
    lv_obj_set_size(top_bar, lv_pct(100), 80);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x111111), 0);
    lv_obj_set_style_bg_grad_color(top_bar, lv_color_hex(0x0), 0);
    lv_obj_set_style_bg_grad_dir(top_bar, LV_GRAD_DIR_VER, 0);

    lv_obj_t *time_label = lv_label_create(top_bar);
    lv_label_set_text(time_label, "12:45 PM");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(time_label, lv_pct(100), lv_pct(100));
    
    lv_obj_t *bottom_bar = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bottom_bar, lv_pct(100), 80);
    lv_obj_align(bottom_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_border_width(bottom_bar, 0, 0);
    lv_obj_set_style_radius(bottom_bar, 0, 0);
    lv_obj_set_style_bg_color(bottom_bar, lv_color_hex(0x222222), 0);
    lv_obj_set_style_bg_grad_color(bottom_bar, lv_color_hex(0x0), 0);
    lv_obj_set_style_bg_grad_dir(bottom_bar, LV_GRAD_DIR_VER, 0);

    lv_obj_t *unlock_label = lv_label_create(bottom_bar);
    lv_label_set_text(unlock_label, "Swipe up to unlock");
    lv_obj_set_style_text_color(unlock_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(unlock_label, LV_ALIGN_CENTER, 0, 0);        
}