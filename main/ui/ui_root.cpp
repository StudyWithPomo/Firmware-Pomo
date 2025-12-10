#include <system/system.hpp>
#include <ui/app_state.hpp>

#include "ui_root.hpp"
#include "lvgl.h"

static lv_obj_t* status_bar;
static lv_obj_t* content_area;
static bool display_time = true;

void ui_toggle_time_display(bool enable)
{
    display_time = enable;
}

lv_obj_t* ui_create_status_label(lv_obj_t* parent, const char* text)
{
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    return label;
}

void ui_root_init()
{
    SystemAPI system = SystemAPI::instance();

    lv_obj_t* root = lv_scr_act();
    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_style_border_width(root, 0, 0);

    status_bar = lv_obj_create(root);
    lv_obj_set_size(status_bar, LV_PCT(100), 24);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_radius(status_bar, 0, 0);
    lv_obj_set_layout(status_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(status_bar, 0, 0);

    if (display_time) {
        std::string time = system.getCurrentTime();
        lv_obj_t* time_label = ui_create_status_label(status_bar, time.c_str());
        lv_obj_set_flex_grow(time_label, 1);
        lv_subject_add_observer_obj(
            &AppState::instance().time_subject, 
            [](lv_observer_t * observer, lv_subject_t * subject){
                lv_obj_t *label = lv_observer_get_target_obj(observer);
                lv_label_set_text(label, lv_subject_get_string(subject));
            },
            time_label,
            NULL
        );
    } else {
        lv_obj_t* spacer = lv_obj_create(status_bar);
        lv_obj_set_flex_grow(spacer, 1);
        lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(spacer, 0, 0);
    }

    SystemAPI::BluetoothStatus bt_status = system.getBluetoothStatus();
    if (bt_status.isEnabled){
        lv_obj_t* bt_label = ui_create_status_label(status_bar, LV_SYMBOL_BLUETOOTH);
    }

    SystemAPI::WiFiStatus wifi_status = system.getWiFiStatus();
    if (wifi_status.isConnected){
        lv_obj_t* wifi_label = ui_create_status_label(status_bar, LV_SYMBOL_WIFI);
    }

    SystemAPI::BatteryStatus battery = system.getBatteryStatus();
    char battery_fmt[5];
    snprintf(battery_fmt, sizeof(battery_fmt), "%d%%", battery.percent);
    lv_obj_t* battery_label = ui_create_status_label(status_bar, battery_fmt);
    lv_subject_add_observer_obj(
        &AppState::instance().battery_subject, 
        [](lv_observer_t * observer, lv_subject_t * subject){
            lv_obj_t *label = lv_observer_get_target_obj(observer);
            char battery_fmt[5];
            snprintf(battery_fmt, sizeof(battery_fmt), "%d%%", static_cast<int>(lv_subject_get_int(subject)));
            lv_label_set_text(label, battery_fmt);
        },
        battery_label,
        NULL
    );

    content_area = lv_obj_create(root);
    lv_obj_set_width(content_area, LV_PCT(100));
    lv_obj_set_flex_grow(content_area, 1);
    lv_obj_set_style_pad_all(content_area, 0, 0);
    lv_obj_set_style_border_width(content_area, 0, 0);
}

lv_obj_t* ui_get_content_area()
{
    return content_area;
}
