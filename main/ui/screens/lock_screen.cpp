#include "lvgl.h"
#include "font/lv_font.h"

#include <ui/screens.hpp>
#include <ui/app_state.hpp>
#include <ui/ui_root.hpp>

#include <ui/wallpapers.h>

#include <system/system.hpp>

void screen_lock(lv_obj_t *parent)
{
    ui_toggle_time_display(false);

    lv_obj_t *bg = lv_img_create(parent);
    lv_obj_set_size(bg, lv_pct(100), lv_pct(100));
    lv_img_set_src(bg, &light);
    lv_obj_set_style_border_width(bg, 0, 0);
    lv_obj_set_style_radius(bg, 0, 0);

    lv_obj_t *date_label_shadow = lv_label_create(parent);
    lv_obj_t *date_label = lv_label_create(parent);
    lv_obj_set_style_text_color(date_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(date_label, LV_ALIGN_TOP_LEFT, 10, 40);

    lv_obj_set_style_text_color(date_label_shadow, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_opa(date_label_shadow, LV_OPA_50, 0);
    lv_obj_align_to(date_label_shadow, date_label, LV_ALIGN_CENTER, 1, 1);

    lv_observer_cb_t date_cb = [](lv_observer_t *observer, lv_subject_t *subject)
    {
        lv_obj_t *label = lv_observer_get_target_obj(observer);
        lv_label_set_text(label, lv_subject_get_string(subject));
    };

    lv_subject_add_observer_obj(
        &AppState::instance().date_subject,
        date_cb,
        date_label,
        NULL);

    lv_subject_add_observer_obj(
        &AppState::instance().date_subject,
        date_cb,
        date_label_shadow,
        NULL);

    lv_obj_t *time_label_shadow = lv_label_create(parent);
    lv_obj_t *time_label = lv_label_create(parent);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_28, 0);
    lv_obj_align_to(time_label, date_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    lv_obj_set_style_text_color(time_label_shadow, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_opa(time_label_shadow, LV_OPA_50, 0);
    lv_obj_set_style_text_font(time_label_shadow, &lv_font_montserrat_28, 0);
    lv_obj_align_to(time_label_shadow, time_label, LV_ALIGN_CENTER, 2, 2);

    lv_observer_cb_t time_cb = [](lv_observer_t *observer, lv_subject_t *subject)
    {
        lv_obj_t *label = lv_observer_get_target_obj(observer);
        lv_label_set_text(label, lv_subject_get_string(subject));
    };

    lv_subject_add_observer_obj(
        &AppState::instance().time_subject,
        time_cb,
        time_label,
        NULL);

    lv_subject_add_observer_obj(
        &AppState::instance().time_subject,
        time_cb,
        time_label_shadow,
        NULL);

    lv_obj_t *bottom_bar = lv_obj_create(parent);
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

    lv_obj_add_event_cb(unlock_slider, [](lv_event_t *e)
                        {
        lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
        // Snap back if released before reaching the end
        if (lv_slider_get_value(slider) < 100) {
            lv_slider_set_value(slider, 0, LV_ANIM_ON);
        } }, LV_EVENT_RELEASED, nullptr);

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

    lv_obj_add_event_cb(unlock_slider, [](lv_event_t *e)
                        {
        lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
        lv_obj_t *custom_knob = (lv_obj_t *)lv_event_get_user_data(e);

        lv_obj_set_pos(
            custom_knob,
            lv_slider_get_value(slider) * (lv_obj_get_width(slider) - lv_obj_get_width(custom_knob)) / 100, 
            (lv_obj_get_height(slider) - lv_obj_get_height(custom_knob)) / 2
        );

        if (lv_slider_get_value(slider) >= 100) {
            lv_slider_set_value(slider, 0, LV_ANIM_ON);
            AppState::instance().unlockPhone();
        } }, LV_EVENT_ALL, custom_knob);
}