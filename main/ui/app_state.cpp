#include "app_state.hpp"
#include <system/system.hpp>

AppState& AppState::instance() {
    static AppState inst;
    return inst;
}

static SystemAPI::CellularStatus cell_status_cache = {};
static long current_time_cache = 0;

AppState::AppState() {
    SystemAPI system = SystemAPI::instance();

    current_time_cache = system.getCurrentTime();
    lv_subject_init_pointer(&time_subject, &current_time_cache);
    
    lv_subject_init_int(&battery_subject, system.getBatteryStatus().percent);

    lv_subject_init_pointer(&cell_subject, &cell_status_cache);
}

void AppState::update() {
    SystemAPI system = SystemAPI::instance();

    const long current_time = system.getCurrentTime();
    if (current_time != current_time_cache) {
        current_time_cache = current_time;
        lv_subject_set_pointer(&time_subject, &current_time_cache);
    }

    const SystemAPI::BatteryStatus battery_status = system.getBatteryStatus();
    if (battery_status.percent != lv_subject_get_int(&battery_subject)) {
        lv_subject_set_int(&battery_subject, battery_status.percent);
    }

    const SystemAPI::CellularStatus cellular_status = system.getCellularStatus();
    if (cellular_status != cell_status_cache) {
        lv_memcpy(&cell_status_cache, &cellular_status, sizeof(SystemAPI::CellularStatus));
        lv_subject_set_pointer(&cell_subject, &cell_status_cache);
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
