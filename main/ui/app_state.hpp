#pragma once
#include "navigator.hpp"

class AppState {
public:
    static AppState& instance();

    void unlockPhone();
    void lockPhone();

private:
    AppState() = default;
};
