#pragma once
#include "lvgl.h"

// Reactive app state. Screens observe these subjects and never read globals directly.
// Subjects are only touched from the LVGL task. The study-session subjects (timer, phase,
// focus/drift, peek) arrive with the session engine in a later milestone.
class AppState {
public:
    static AppState& instance();

    // Pointer subject to the current epoch seconds (long)
    lv_subject_t time_subject;

    // Pet (set from the laptop's config message or the Workshop)
    lv_subject_t pet_name;      // string, up to 10 letters
    lv_subject_t pet_body;      // PetBody
    lv_subject_t pet_face;      // PetFace
    lv_subject_t pet_finish;    // PetFinish

    // Progress (the device is the source of truth)
    lv_subject_t xp;            // total XP
    lv_subject_t level;         // xp / 500 + 1

    // Study circle
    lv_subject_t multiplier;    // 1..3
    lv_subject_t circle_names;  // string, e.g. "Ava, Dev"; empty when alone

    // Timer settings
    lv_subject_t study_min;     // 15 / 25 / 45 / 60
    lv_subject_t break_min;

    // Call from the LVGL task (an lv_timer) once a second
    void update();

private:
    AppState();
};
