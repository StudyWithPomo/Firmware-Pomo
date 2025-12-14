#include <system/system.hpp>
#include <ui/app_state.hpp>
#include <ui/fonts.h>

#include "ui_root.hpp"
#include "lvgl.h"
#include <cstring>

static lv_obj_t *status_bar;
static lv_obj_t *content_area;
static lv_subject_t display_time_subject;

void ui_toggle_time_display(bool enable)
{
    lv_subject_set_int(&display_time_subject, enable ? 1 : 0);
}

lv_obj_t *ui_create_status_label(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    return label;
}

const char *get_battery_symbol(int percent)
{
    if (percent >= 90)
    {
        return LV_SYMBOL_BATTERY_FULL;
    }
    else if (percent >= 65)
    {
        return LV_SYMBOL_BATTERY_3;
    }
    else if (percent >= 40)
    {
        return LV_SYMBOL_BATTERY_2;
    }
    else if (percent >= 15)
    {
        return LV_SYMBOL_BATTERY_1;
    }
    else
    {
        return LV_SYMBOL_BATTERY_EMPTY;
    }
}

const char *get_signal_symbol(int rssi)
{
    if (rssi >= -30)
    {
        return UI_SYMBOL_SIGNAL;
    }
    else if (rssi >= -67)
    {
        return UI_SYMBOL_SIGNAL_3;
    }
    else if (rssi >= -70)
    {
        return UI_SYMBOL_SIGNAL_2;
    }
    else if (rssi >= -80)
    {
        return UI_SYMBOL_SIGNAL_1;
    }
    else
    {
        return "";
    }
}

void ui_root_init()
{
    SystemAPI system = SystemAPI::instance();
    lv_subject_init_int(&display_time_subject, 1);

    lv_obj_t *root = lv_scr_act();
    lv_obj_set_style_bg_color(root, lv_color_hex(0x000000), 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_style_border_width(root, 0, 0);

    content_area = lv_obj_create(root);
    lv_obj_set_size(content_area, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(content_area, 0, 0);
    lv_obj_set_style_border_width(content_area, 0, 0);
    lv_obj_set_style_radius(content_area, 0, 0);
    lv_obj_set_style_bg_opa(content_area, LV_OPA_TRANSP, 0);

    status_bar = lv_obj_create(root);
    lv_obj_set_size(status_bar, LV_PCT(100), 24);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(status_bar, LV_OPA_70, 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_radius(status_bar, 0, 0);
    lv_obj_set_layout(status_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_ver(status_bar, 0, 0);
    lv_obj_set_style_pad_hor(status_bar, 5, 0);
    lv_obj_set_scroll_dir(status_bar, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(status_bar, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *cell_label = ui_create_status_label(status_bar, "...");
    lv_obj_set_style_text_font(cell_label, &ext_symbols_14, 0);
    lv_subject_add_observer_obj(
        &AppState::instance().cell_subject,
        [](lv_observer_t *observer, lv_subject_t *subject)
        {
            lv_obj_t *label = lv_observer_get_target_obj(observer);
            SystemAPI::CellularStatus *status = (SystemAPI::CellularStatus *)lv_subject_get_pointer(subject);

            if (status->simCardPresent)
            {
                if (status->signalStrength == 0)
                {
                    lv_label_set_text(label, "No Signal");
                }
                else
                {
                    lv_label_set_text_fmt(label, "%s %s", get_signal_symbol(status->signalStrength), status->carrierName);
                }
            }
            else
            {
                lv_label_set_text(label, "No SIM");
            }
        },
        cell_label,
        NULL);

    lv_obj_t *spacer = lv_obj_create(status_bar);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);

    lv_obj_t *battery_label = ui_create_status_label(status_bar, "");
    lv_subject_add_observer_obj(
        &AppState::instance().battery_subject,
        [](lv_observer_t *observer, lv_subject_t *subject)
        {
            lv_obj_t *label = lv_observer_get_target_obj(observer);
            int percent = lv_subject_get_int(subject);
            lv_label_set_text_fmt(label, "%s %d%%", get_battery_symbol(percent), percent);
        },
        battery_label,
        NULL);

    std::string time = system.getCurrentTimeFormatted();
    lv_obj_t *time_label = ui_create_status_label(status_bar, time.c_str());
    lv_obj_add_flag(time_label, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 0);
    lv_subject_add_observer_obj(
        &AppState::instance().time_subject,
        [](lv_observer_t *observer, lv_subject_t *subject)
        {
            lv_obj_t *label = lv_observer_get_target_obj(observer);
            lv_label_set_text(label, lv_subject_get_string(subject));
        },
        time_label,
        NULL);

    lv_subject_add_observer_obj(
        &display_time_subject,
        [](lv_observer_t *observer, lv_subject_t *subject)
        {
            lv_obj_t *label = lv_observer_get_target_obj(observer);
            bool show = lv_subject_get_int(subject);

            if (show)
            {
                lv_obj_set_style_opa(label, LV_OPA_COVER, 0);
            }
            else
            {
                lv_obj_set_style_opa(label, LV_OPA_TRANSP, 0);
            }
        },
        time_label,
        NULL);

    if (lv_subject_get_int(&display_time_subject))
    {
        lv_obj_clear_flag(time_label, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(time_label, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t *ui_get_content_area()
{
    return content_area;
}
