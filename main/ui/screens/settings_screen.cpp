#include <cstring>
#include <ctime>

#include <system/system.hpp>
#include <ui/app_state.hpp>
#include <ui/screens.hpp>
#include <utilities.h>

#include "esp_log.h"

static constexpr const char *kb_numpad_map[] = {
    "1", "2", "3", "\n",
    "4", "5", "6", "\n",
    "7", "8", "9", "\n",
    LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, NULL};

static constexpr const lv_buttonmatrix_ctrl_t kb_numpad_ctrl[] =
    {
        LV_BUTTONMATRIX_CTRL_WIDTH_2, LV_BUTTONMATRIX_CTRL_WIDTH_2, LV_BUTTONMATRIX_CTRL_WIDTH_2,
        LV_BUTTONMATRIX_CTRL_WIDTH_2, LV_BUTTONMATRIX_CTRL_WIDTH_2, LV_BUTTONMATRIX_CTRL_WIDTH_2,
        LV_BUTTONMATRIX_CTRL_WIDTH_2, LV_BUTTONMATRIX_CTRL_WIDTH_2, LV_BUTTONMATRIX_CTRL_WIDTH_2,
        LV_BUTTONMATRIX_CTRL_WIDTH_2, LV_BUTTONMATRIX_CTRL_WIDTH_2, LV_BUTTONMATRIX_CTRL_WIDTH_2};

#define KB_HEIGHT 40

void set_time_cb(lv_event_t *e)
{
    lv_obj_t *time_input = (lv_obj_t *)lv_event_get_user_data(e);

    const char *time_str = lv_textarea_get_text(time_input);
    if (strlen(time_str) != 4)
    {
        // Invalid input length
        ESP_LOGI("SET_TIME", "Invalid input length: %s", time_str);
        lv_obj_set_style_outline_color(time_input, lv_color_hex(0xFF0000), 0);
        return;
    }

    int hour = (time_str[0] - '0') * 10 + (time_str[1] - '0');
    int minute = (time_str[2] - '0') * 10 + (time_str[3] - '0');

    if (hour < 0 || hour > 23 || minute < 0 || minute > 59)
    {
        // Invalid time
        ESP_LOGI("SET_TIME", "Invalid time: %s", time_str);
        lv_obj_set_style_outline_color(time_input, lv_color_hex(0xFF0000), 0);
        return;
    }

    SystemAPI system = SystemAPI::instance();
    const time_t current_time = system.getCurrentTime();
    std::tm *tm_info = std::localtime(&current_time);
    tm_info->tm_hour = hour;
    tm_info->tm_min = minute;
    tm_info->tm_sec = 0;

    long new_epoch = std::mktime(tm_info);
    if (new_epoch != -1)
    {
        system.setCurrentTime(new_epoch);
    }
    else
    {
        ESP_LOGI("SET_TIME", "mktime failed for time: %s", time_str);
        lv_obj_set_style_outline_color(time_input, lv_color_hex(0xFF0000), 0);
    }
}

void set_date_cb(lv_event_t *e)
{
    lv_obj_t *date_input = (lv_obj_t *)lv_event_get_user_data(e);
    const char *date_str = lv_textarea_get_text(date_input);
    if (strlen(date_str) != 8)
    {
        // Invalid input length
        ESP_LOGI("SET_DATE", "Invalid input length: %s", date_str);
        lv_obj_set_style_outline_color(date_input, lv_color_hex(0xFF0000), 0);
        return;
    }
    int month = (date_str[0] - '0') * 10 + (date_str[1] - '0');
    int day = (date_str[2] - '0') * 10 + (date_str[3] - '0');
    int year = (date_str[4] - '0') * 1000 + (date_str[5] - '0') * 100 + (date_str[6] - '0') * 10 + (date_str[7] - '0');
    if (month < 1 || month > 12 || day < 1 || day > 31 || year < 1970)
    {
        // Invalid date
        ESP_LOGI("SET_DATE", "Invalid date: %s", date_str);
        lv_obj_set_style_outline_color(date_input, lv_color_hex(0xFF0000), 0);
        return;
    }
    SystemAPI system = SystemAPI::instance();
    const time_t current_time = system.getCurrentTime();
    std::tm *tm_info = std::localtime(&current_time);
    tm_info->tm_year = year - 1900;
    tm_info->tm_mon = month - 1;
    tm_info->tm_mday = day;
    long new_epoch = std::mktime(tm_info);
    if (new_epoch != -1)
    {
        system.setCurrentTime(new_epoch);
    }
    else
    {
        ESP_LOGI("SET_DATE", "mktime failed for date: %s", date_str);
        lv_obj_set_style_outline_color(date_input, lv_color_hex(0xFF0000), 0);
    }
}

lv_obj_t *wifi_settings_page(lv_obj_t *parent)
{
    lv_obj_t *page = lv_menu_page_create(parent, NULL);
    lv_obj_t *cont = lv_menu_cont_create(page);
    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, "WiFi Settings");

    return page;
}

lv_obj_t *bluetooth_settings_page(lv_obj_t *parent)
{
    lv_obj_t *page = lv_menu_page_create(parent, NULL);
    lv_obj_t *cont = lv_menu_cont_create(page);
    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, "Bluetooth Settings");

    return page;
}

lv_obj_t *date_time_settings_page(lv_obj_t *parent)
{
    AppState appState = AppState::instance();

    lv_obj_t *page = lv_menu_page_create(parent, NULL);
    lv_obj_set_scroll_dir(page, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_AUTO);

    lv_obj_t *cont = lv_menu_cont_create(page);
    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, "Date and Time Settings");

    lv_obj_t *keyboard = lv_keyboard_create(lv_layer_top());
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_4, kb_numpad_map, kb_numpad_ctrl);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_4);
    lv_obj_set_size(keyboard, lv_pct(100), lv_pct(KB_HEIGHT));
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_menu_separator_create(page);

    cont = lv_menu_cont_create(page);
    lv_obj_t *time_label = lv_label_create(cont);
    lv_subject_add_observer_obj(
        &appState.time_subject,
        [](lv_observer_t *observer, lv_subject_t *subject)
        {
            lv_obj_t *label = lv_observer_get_target_obj(observer);
            const char *buffer = format_time(*(long *)lv_subject_get_pointer(subject), "%I:%M %p");
            lv_label_set_text_fmt(label, "Current Time: %s", buffer);
        },
        time_label,
        NULL);

    cont = lv_menu_cont_create(page);
    lv_obj_t *time_input = lv_textarea_create(cont);
    lv_obj_set_width(time_input, lv_pct(50));
    lv_textarea_set_placeholder_text(time_input, "HHMM");
    lv_textarea_set_accepted_chars(time_input, "0123456789");
    lv_textarea_set_max_length(time_input, 4);
    lv_textarea_set_one_line(time_input, true);
    lv_obj_add_event_cb(time_input, [](lv_event_t *e)
                        {
            lv_obj_t *ta = lv_event_get_target_obj(e);
            lv_obj_t *keyboard = (lv_obj_t *)lv_event_get_user_data(e);
            lv_obj_remove_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
            lv_keyboard_set_textarea(keyboard, ta);

            lv_obj_t *cont = lv_obj_get_parent(ta);
            lv_obj_t *parent = lv_obj_get_parent(cont);
            
            lv_coord_t ta_y = lv_obj_get_y(cont) + lv_obj_get_height(cont);
            lv_coord_t parent_h = lv_obj_get_height(parent);
            lv_coord_t kb_h = lv_obj_get_height(keyboard);

            if (ta_y > parent_h - kb_h - 50) {
                lv_obj_scroll_to_y(parent, ta_y - (parent_h - kb_h - 50), LV_ANIM_ON);
            } }, LV_EVENT_FOCUSED, keyboard);

    lv_obj_add_event_cb(time_input, [](lv_event_t *e)
                        {
                            lv_obj_t *keyboard = (lv_obj_t *)lv_event_get_user_data(e);
                            lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
                            lv_keyboard_set_textarea(keyboard, NULL); }, LV_EVENT_DEFOCUSED, keyboard);

    lv_obj_t *set_time_btn = lv_btn_create(cont);
    lv_obj_set_width(set_time_btn, lv_pct(40));
    lv_obj_add_event_cb(set_time_btn, set_time_cb, LV_EVENT_CLICKED, time_input);

    lv_obj_t *btn_label = lv_label_create(set_time_btn);
    lv_label_set_text(btn_label, "Set Time");
    lv_obj_center(btn_label);

    lv_menu_separator_create(page);

    cont = lv_menu_cont_create(page);
    lv_obj_t *date_label = lv_label_create(cont);
    lv_subject_add_observer_obj(
        &appState.time_subject,
        [](lv_observer_t *observer, lv_subject_t *subject)
        {
            lv_obj_t *label = lv_observer_get_target_obj(observer);
            const char *buffer = format_time(*(long *)lv_subject_get_pointer(subject), "%b %d, %Y");
            lv_label_set_text_fmt(label, "Current Date: %s", buffer);
        },
        date_label,
        NULL);

    cont = lv_menu_cont_create(page);
    lv_obj_t *date_input = lv_textarea_create(cont);
    lv_obj_set_width(date_input, lv_pct(50));
    lv_textarea_set_placeholder_text(date_input, "MMDDYYYY");
    lv_textarea_set_accepted_chars(date_input, "0123456789");
    lv_textarea_set_max_length(date_input, 8);
    lv_textarea_set_one_line(date_input, true);
    lv_obj_add_event_cb(date_input, [](lv_event_t *e)
                        {
                            lv_obj_t *ta = lv_event_get_target_obj(e);
                            lv_obj_t *keyboard = (lv_obj_t *)lv_event_get_user_data(e);
                            lv_obj_remove_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
                            lv_keyboard_set_textarea(keyboard, ta);

                            lv_obj_t *cont = lv_obj_get_parent(ta);
                            lv_obj_t *parent = lv_obj_get_parent(cont);

                            lv_coord_t ta_y = lv_obj_get_y(cont) + lv_obj_get_height(cont);
                            lv_coord_t parent_h = lv_obj_get_height(parent);
                            lv_coord_t kb_h = lv_obj_get_height(keyboard);

                            if (ta_y > parent_h - kb_h - 50)
                            {
                                lv_obj_scroll_to_y(parent, ta_y - (parent_h - kb_h - 50), LV_ANIM_ON);
                            } }, LV_EVENT_FOCUSED, keyboard);
    lv_obj_add_event_cb(date_input, [](lv_event_t *e)
                        {
        lv_obj_t *keyboard = (lv_obj_t *)lv_event_get_user_data(e);
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_keyboard_set_textarea(keyboard, NULL); }, LV_EVENT_DEFOCUSED, keyboard);

    lv_obj_t *set_date_btn = lv_btn_create(cont);
    lv_obj_set_width(set_date_btn, lv_pct(40));
    lv_obj_add_event_cb(set_date_btn, set_date_cb, LV_EVENT_CLICKED, date_input);

    lv_obj_t *date_btn_label = lv_label_create(set_date_btn);
    lv_label_set_text(date_btn_label, "Set Date");
    lv_obj_center(date_btn_label);

    cont = lv_menu_cont_create(page);
    lv_obj_set_height(cont, 200); // Spacer to allow scrolling above keyboard

    return page;
}

void screen_settings(lv_obj_t *parent)
{
    lv_obj_t *bg = lv_obj_create(parent);
    lv_obj_set_size(bg, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(bg, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(bg, 0, 0);
    lv_obj_set_style_radius(bg, 0, 0);
    lv_obj_set_style_pad_all(bg, 0, 0);
    lv_obj_set_style_pad_top(bg, 24, 0); // Leave space for status bar

    lv_obj_t *menu = lv_menu_create(bg);
    lv_obj_set_size(menu, lv_pct(100), lv_pct(100));
    lv_obj_add_event_cb(menu, [](lv_event_t *e)
                        {        
        lv_obj_t * obj = lv_event_get_target_obj(e);
        lv_obj_t * menu = (lv_obj_t *)lv_event_get_user_data(e);
        if (lv_menu_back_button_is_root(menu, obj)) {
            AppState::instance().returnToHome();
        } }, LV_EVENT_CLICKED, menu);

    lv_menu_set_mode_root_back_button(menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);

    lv_obj_t *cont;
    lv_obj_t *label;

    lv_obj_t *main_page = lv_menu_page_create(menu, NULL);

    cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_label_set_text_fmt(label, "%s WiFi Settings", LV_SYMBOL_WIFI);
    lv_menu_set_load_page_event(menu, cont, wifi_settings_page(menu));

    cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_label_set_text_fmt(label, "%s Bluetooth Settings", LV_SYMBOL_BLUETOOTH);
    lv_menu_set_load_page_event(menu, cont, bluetooth_settings_page(menu));

    cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_label_set_text(label, "Date and Time");
    lv_menu_set_load_page_event(menu, cont, date_time_settings_page(menu));

    lv_menu_set_page(menu, main_page);
}