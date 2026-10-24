#include "app_state.hpp"
#include <system/system.hpp>

AppState& AppState::instance() {
    static AppState inst;
    return inst;
}

static long current_time_cache = 0;

AppState::AppState() {
    current_time_cache = SystemAPI::instance().getCurrentTime();
    lv_subject_init_pointer(&time_subject, &current_time_cache);
}

void AppState::update() {
    const long current_time = SystemAPI::instance().getCurrentTime();
    if (current_time != current_time_cache) {
        current_time_cache = current_time;
        lv_subject_set_pointer(&time_subject, &current_time_cache);
    }
}
