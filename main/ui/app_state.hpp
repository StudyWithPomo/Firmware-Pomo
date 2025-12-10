#pragma once
#include "navigator.hpp"
#include "lvgl.h"

class AppState {
public:
    static AppState& instance();

    lv_subject_t time_subject;
    lv_subject_t date_subject;
    lv_subject_t battery_subject;

    void unlockPhone();
    void lockPhone();

    void update();

private:
    AppState();
};
