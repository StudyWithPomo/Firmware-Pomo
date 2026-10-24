#pragma once
#include "lvgl.h"
#include <app/session_core.hpp>

// Reactive app state. Screens observe these subjects and never read globals directly.
// Subjects are only touched from the LVGL task: background work (the session timer, the
// laptop link, the study circle) reaches them through queues drained by an lv_timer here.
class AppState {
public:
    static AppState& instance();

    struct Friend {
        char name[11];
        bool focused;
        int pct;
    };
    static constexpr int MAX_FRIENDS = 4;

    // Pointer subject to the current epoch seconds (long)
    lv_subject_t time_subject;
    lv_subject_t user_name;         // string; empty until the laptop sends it

    // Pet
    lv_subject_t pet_name;          // string, up to 10 letters
    lv_subject_t pet_body;          // PetBody
    lv_subject_t pet_face;          // PetFace
    lv_subject_t pet_finish;        // PetFinish
    lv_subject_t pet_mood;          // PetMood, follows the session (and a tap's "shh")

    // Progress (the device is the source of truth)
    lv_subject_t xp;
    lv_subject_t level;

    // Study circle
    lv_subject_t multiplier;        // effective right now: 1 while anyone drifts
    lv_subject_t base_multiplier;   // the circle's bonus when nobody drifts ("2x paused")
    lv_subject_t circle_names;      // string, e.g. "Ava, Dev"; empty when alone
    lv_subject_t circle_version;    // bumps whenever friends() changes

    // Timer settings
    lv_subject_t study_min;         // 15 / 25 / 45 / 60
    lv_subject_t break_min;
    lv_subject_t always_show_timer; // 0/1

    // Session
    lv_subject_t phase;             // Phase
    lv_subject_t session_index;     // 1-4
    lv_subject_t timer_remaining_s;
    lv_subject_t timer_total_s;
    lv_subject_t focus_state;       // 0 focused, 1 drift
    lv_subject_t drift_label;       // string, e.g. "YouTube"
    lv_subject_t drift_seconds;
    lv_subject_t peek_active;       // 0/1, the timer is lit for 5 s

    // Start draining the session queues (call once, LVGL lock held, after Session::init)
    void attach();
    // Call from the LVGL task once a second
    void update();

    // ---- Actions (LVGL task) ----
    bool startSession();                    // with the current study/break lengths
    bool startSession(int study, int brk);
    void endSessionEarly();
    void peek();                            // light the timer for 5 s
    void shh();                             // the pet hushes for 2 s
    void setStudyLength(int study_min);     // keeps the break in range
    void setBreakLength(int break_min);
    void setPet(int body, int face, int finish);
    void setFriends(const Friend *friends, int count);

    const Friend *friends(int *count) const { *count = friend_count; return friend_list; }
    const SessionResult &lastResult() const { return last_result; }

private:
    AppState();
    static void drain(lv_timer_t *t);
    void applySnapshot(const SessionSnapshot &snap);
    void handleEvent(const SessionEvent &event);
    void refreshMood();

    Friend friend_list[MAX_FRIENDS] = {};
    int friend_count = 0;
    SessionResult last_result = {};
    lv_timer_t *peek_timer = nullptr;
    lv_timer_t *shh_timer = nullptr;
    bool shh_active = false;
};
