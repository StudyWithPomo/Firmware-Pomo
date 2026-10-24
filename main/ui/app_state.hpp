#pragma once
#include "lvgl.h"

// Reactive app state. Screens observe these subjects and never read globals directly.
// Rewritten for the study flow in a later milestone; for now it only tracks the clock.
class AppState {
public:
    static AppState& instance();

    // Pointer subject to the current epoch seconds (long)
    lv_subject_t time_subject;

    // Call from the LVGL task (an lv_timer) once a second
    void update();

private:
    AppState();
};
