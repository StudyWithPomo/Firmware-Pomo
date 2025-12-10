#include "app_state.hpp"
#include <system/system.hpp>

AppState& AppState::instance() {
    static AppState inst;
    return inst;
}

static char time_buf[12] = {0};
static char date_buf[32] = {0};
static SystemAPI::CellularStatus cell_status_cache = {};

AppState::AppState() {
    SystemAPI system = SystemAPI::instance();

    lv_subject_init_string(&time_subject, time_buf, nullptr, sizeof(time_buf), system.getCurrentTime().c_str());

    lv_subject_init_string(&date_subject, date_buf, nullptr, sizeof(date_buf), system.getCurrentDate().c_str());
    
    lv_subject_init_int(&battery_subject, system.getBatteryStatus().percent);

    lv_subject_init_pointer(&cell_subject, &cell_status_cache);
}

void AppState::update() {
    SystemAPI system = SystemAPI::instance();

    const std::string current_time = system.getCurrentTime();
    if (current_time != lv_subject_get_string(&time_subject)) {
        lv_subject_copy_string(&time_subject, current_time.c_str());        
    }

    const std::string current_date = system.getCurrentDate();
    if (current_date !=  lv_subject_get_string(&date_subject)) {
        lv_subject_copy_string(&date_subject, current_date.c_str());
    }

    const SystemAPI::BatteryStatus battery_status = system.getBatteryStatus();
    if (battery_status.percent != lv_subject_get_int(&battery_subject)) {
        lv_subject_set_int(&battery_subject, battery_status.percent);
    }

    const SystemAPI::CellularStatus cellular_status = system.getCellularStatus();
    if (cellular_status != cell_status_cache) {
        lv_memcpy(&cell_status_cache, &cellular_status, sizeof(SystemAPI::CellularStatus));
        lv_subject_notify(&cell_subject);
    }
}

void AppState::returnToHome() {
    Navigator::instance().navigate(ScreenID::HOME);
}

void AppState::openSettings() {
    Navigator::instance().navigate(ScreenID::SETTINGS);
}

void AppState::unlockPhone() {
    Navigator::instance().navigate(ScreenID::HOME);
}

void AppState::lockPhone() {
    Navigator::instance().navigate(ScreenID::LOCK);
}

