#pragma once
#include "navigator.hpp"
#include "lvgl.h"

class AppState {
public:
    static AppState& instance();

    lv_subject_t time_subject;
    lv_subject_t date_subject;
    lv_subject_t battery_subject;

    lv_subject_t cell_subject;

    void unlockPhone();
    void lockPhone();

    void returnToHome();
    void openSettings();

    void update();

private:
    AppState();
};
