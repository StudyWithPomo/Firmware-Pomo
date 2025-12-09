#include "lvgl.h"
#include "status_bar.hpp"
#include <system/system.hpp>

void render_status_bar(bool show_time) {
    const SystemAPI *system = SystemAPI::getInstance();

    lv_obj_t *bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bg, lv_pct(100), lv_pct(100));
    lv_obj_set_style_border_width(bg, 0, 0);
    lv_obj_set_style_radius(bg, 0, 0);
    lv_obj_set_style_bg_color(bg, lv_color_hex(0x0), 0);

    int battery_level = system->getBatteryStatus().percent;
    const char *battery_symbol;
    switch (battery_level / 25) {
        case 4:
            battery_symbol = LV_SYMBOL_BATTERY_FULL;
            break;
        case 3:
            battery_symbol = LV_SYMBOL_BATTERY_3;
            break;
        case 2:
            battery_symbol = LV_SYMBOL_BATTERY_2;
            break;
        case 1:
            battery_symbol = LV_SYMBOL_BATTERY_1;
            break;
        default:
            battery_symbol = LV_SYMBOL_BATTERY_EMPTY;
            break;
    }

    lv_obj_t *battery_icon = lv_label_create(lv_scr_act());
    lv_label_set_text(battery_icon, battery_symbol);
    lv_obj_set_style_text_color(battery_icon, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(battery_icon, LV_ALIGN_TOP_RIGHT, -10, 10);

    auto wifi_data = system->getWiFiStatus();
    if (wifi_data.isConnected) {
        lv_obj_t *signal_icon = lv_label_create(lv_scr_act());
        lv_label_set_text(signal_icon, LV_SYMBOL_WIFI);
        lv_obj_set_style_text_color(signal_icon, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(signal_icon, LV_ALIGN_TOP_RIGHT, -40, 10);
    }

    auto bluetooth_data = system->getBluetoothStatus();
    if (bluetooth_data.isEnabled) {
        lv_obj_t *bt_icon = lv_label_create(lv_scr_act());
        lv_label_set_text(bt_icon, LV_SYMBOL_BLUETOOTH);
        lv_obj_set_style_text_color(bt_icon, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(bt_icon, LV_ALIGN_TOP_RIGHT, -70, 10);
    }

    if (show_time) {
        auto time = system->getCurrentTime();

        lv_obj_t *time_label = lv_label_create(lv_scr_act());
        lv_label_set_text(time_label, time.c_str());
        lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(time_label, LV_ALIGN_TOP_MID, 0, 10);
    }
}