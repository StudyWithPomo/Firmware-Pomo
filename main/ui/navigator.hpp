#pragma once

#include <functional>
#include <map>

#include "lvgl.h"
#include "screens.hpp"

typedef void (*ScreenBuilder)(lv_obj_t* parent);

class Navigator {
public:
    Navigator(std::map<ScreenID, ScreenBuilder> registry)
        : registry(registry) {}

    static Navigator& instance() {
        static Navigator inst({
            {ScreenID::LOCK, screen_lock},
            {ScreenID::HOME, screen_home},
            {ScreenID::PHONE, screen_phone},
            {ScreenID::SMS, screen_sms},
            {ScreenID::CONTACTS, screen_contacts},
            {ScreenID::SETTINGS, screen_settings}
        });
        return inst;
    }

    void navigate(ScreenID id);

    ScreenID getCurrent() const {
        return current;
    }

private:
    ScreenID current;
    std::map<ScreenID, ScreenBuilder> registry;
};
