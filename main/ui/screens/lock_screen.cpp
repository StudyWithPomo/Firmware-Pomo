#include <ui/screen.hpp>
#include <ui/ui_manager.hpp>
#include "home_screen.hpp"
#include "lock_screen.hpp"
#include "lvgl.h"
#include "font/lv_font.h"

#include "status_bar.hpp"

#include <system/system.hpp>

void LockScreen::render() {
    const SystemAPI *system = SystemAPI::getInstance();

    render_status_bar(false);

    auto date = system->getCurrentDate();

    lv_obj_t *date_label = lv_label_create(lv_scr_act());
    lv_label_set_text(date_label, date.c_str());
    lv_obj_set_style_text_color(date_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(date_label, LV_ALIGN_TOP_LEFT, 10, 40);

    auto time = system->getCurrentTime();

    lv_obj_t *time_label = lv_label_create(lv_scr_act());
    lv_label_set_text(time_label, time.c_str());
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(time_label, LV_ALIGN_TOP_LEFT, 10, 60);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_28, 0);
    
    lv_obj_t *bottom_bar = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bottom_bar, lv_pct(100), 80);
    lv_obj_align(bottom_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_border_width(bottom_bar, 0, 0);
    lv_obj_set_style_radius(bottom_bar, 0, 0);
    lv_obj_set_style_bg_color(bottom_bar, lv_color_hex(0x222222), 0);
    lv_obj_set_style_bg_grad_color(bottom_bar, lv_color_hex(0x0), 0);
    lv_obj_set_style_bg_grad_dir(bottom_bar, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_grad_stop(bottom_bar, 128, 0);

    lv_obj_t *unlock_slider = lv_slider_create(bottom_bar);
    lv_obj_set_size(unlock_slider, lv_pct(90), 40);
    lv_obj_align(unlock_slider, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(unlock_slider, LV_OBJ_FLAG_ADV_HITTEST);
    lv_slider_set_value(unlock_slider, 0, LV_ANIM_OFF);

    lv_obj_add_event_cb(unlock_slider, [](lv_event_t * e) {
        lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
        // Snap back if released before reaching the end
        if (lv_slider_get_value(slider) < 100) {
            lv_slider_set_value(slider, 0, LV_ANIM_ON);
        }
    }, LV_EVENT_RELEASED, nullptr);

    lv_obj_set_style_bg_color(unlock_slider, lv_color_hex(0x444444), 0);
    lv_obj_set_style_bg_grad_color(unlock_slider, lv_color_hex(0x666666), 0);
    lv_obj_set_style_bg_grad_dir(unlock_slider, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(unlock_slider, 10, 0);
    lv_obj_set_style_border_width(unlock_slider, 0, 0);
    lv_obj_set_style_pad_hor(unlock_slider, 20, 0);

    lv_obj_set_style_bg_opa(unlock_slider, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(unlock_slider, LV_OPA_TRANSP, LV_PART_KNOB);

    lv_obj_t *custom_knob = lv_label_create(unlock_slider);
    lv_label_set_text(custom_knob, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(custom_knob, lv_color_hex(0xFFFFFF), 0);

    lv_obj_add_event_cb(unlock_slider, [](lv_event_t * e) {
        lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
        lv_obj_t *custom_knob = (lv_obj_t *)lv_event_get_user_data(e);

        if (lv_slider_get_value(slider) >= 100) {
            lv_slider_set_value(slider, 0, LV_ANIM_ON);
            UIManager::getInstance().setScreen(HomeScreen::getInstance());
        }

        lv_obj_set_pos(
            custom_knob,
            lv_slider_get_value(slider) * (lv_obj_get_width(slider) - lv_obj_get_width(custom_knob)) / 100, 
            (lv_obj_get_height(slider) - lv_obj_get_height(custom_knob)) / 2
        );
    }, LV_EVENT_ALL, custom_knob);
}