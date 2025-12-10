#include "app_state.hpp"

AppState& AppState::instance() {
    static AppState inst;
    return inst;
}

void AppState::unlockPhone() {
    Navigator::instance().navigate(ScreenID::HOME);
}

void AppState::lockPhone() {
    Navigator::instance().navigate(ScreenID::LOCK);
}

