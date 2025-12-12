#include <system/system.hpp>

#include <ui/app_state.hpp>
#include <ui/screens.hpp>

lv_obj_t *wifi_settings_page(lv_obj_t* parent) {
    lv_obj_t *page = lv_menu_page_create(parent, NULL);
    lv_obj_t *cont = lv_menu_cont_create(page);
    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, "WiFi Settings");

    return page;
}

lv_obj_t *bluetooth_settings_page(lv_obj_t* parent) {
    lv_obj_t *page = lv_menu_page_create(parent, NULL);
    lv_obj_t *cont = lv_menu_cont_create(page);
    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, "Bluetooth Settings");

    return page;
}

void screen_settings(lv_obj_t* parent) {
    SystemAPI system = SystemAPI::instance();

    lv_obj_t* bg = lv_obj_create(parent);
    lv_obj_set_size(bg, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(bg, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(bg, 0, 0);
    lv_obj_set_style_radius(bg, 0, 0);
    lv_obj_set_style_pad_all(bg, 0, 0);
    lv_obj_set_style_pad_top(bg, 24, 0); // Leave space for status bar

    lv_obj_t* menu = lv_menu_create(bg);
    lv_obj_set_size(menu, lv_pct(100), lv_pct(100));
    lv_obj_add_event_cb(menu, [](lv_event_t* e) {        
        lv_obj_t * obj = lv_event_get_target_obj(e);
        lv_obj_t * menu = (lv_obj_t *)lv_event_get_user_data(e);
        if (lv_menu_back_button_is_root(menu, obj)) {
            AppState::instance().returnToHome();
        }
    }, LV_EVENT_CLICKED, menu);

    lv_menu_set_mode_root_back_button(menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);

    lv_obj_t * cont;
    lv_obj_t * label;

    lv_obj_t * main_page = lv_menu_page_create(menu, NULL);

    cont = lv_menu_cont_create(main_page);
    lv_obj_t *wifi_box = lv_menu_cont_create(cont);
    label = lv_label_create(wifi_box);
    lv_label_set_text_fmt(label, "%s WiFi Settings", LV_SYMBOL_WIFI);
    lv_menu_set_load_page_event(menu, cont, wifi_settings_page(menu));

    cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_label_set_text_fmt(label, "%s Bluetooth Settings", LV_SYMBOL_BLUETOOTH);
    lv_menu_set_load_page_event(menu, cont, bluetooth_settings_page(menu));

    lv_menu_set_page(menu, main_page);
}