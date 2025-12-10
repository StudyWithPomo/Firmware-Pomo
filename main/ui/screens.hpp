#pragma once
#include <lvgl.h>

enum ScreenID {
    LOCK,
    HOME,
    PHONE,
    SMS,
    CONTACTS,
    SETTINGS
};

void screen_lock(lv_obj_t* parent);
void screen_home(lv_obj_t* parent);

void screen_phone(lv_obj_t* parent);
void screen_sms(lv_obj_t* parent);
void screen_contacts(lv_obj_t* parent);
void screen_settings(lv_obj_t* parent);