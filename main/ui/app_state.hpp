#pragma once
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <app/session_core.hpp>
#include <app/storage.hpp>

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

    // A change requested by another task (the laptop link), applied in the LVGL task
    struct UiCommand {
        enum Type : uint8_t { CONFIG, PEEK, LINK_SEEN } type;
        bool has_user, has_pet_name, has_time, has_tz;
        char user[17];
        char pet_name[11];
        int8_t body, face, finish;     // -1 = unchanged
        long epoch;
        int tz_minutes;                // east of UTC
    };

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
    lv_subject_t test_timing;       // 0/1: sessions started here use 30 s phases. Not saved,
                                    // so it's off again after a restart

    // Session
    lv_subject_t phase;             // Phase (READY: the break ran out, waiting for the user)
    lv_subject_t session_index;     // 1-4
    lv_subject_t session_test;      // 0/1: the running session uses test timing
    lv_subject_t timer_remaining_s;
    lv_subject_t timer_total_s;
    lv_subject_t focus_state;       // 0 focused, 1 drift
    lv_subject_t drift_label;       // string, e.g. "YouTube"
    lv_subject_t drift_seconds;
    lv_subject_t peek_active;       // 0/1, the timer is lit for 5 s

    // Laptop link
    lv_subject_t link_connected;    // 1 once the laptop has sent a valid message

    // Study log: bumps whenever log() changes (screens showing "today" also watch time_subject)
    lv_subject_t log_version;

    // Before the laptop sets the clock the RTC counts from 1970
    static constexpr long CLOCK_SET_AFTER = 1700000000;

    // Start draining the session queues (call once, LVGL lock held, after Session::init)
    void attach();
    // Restore saved progress and settings, then save changes from now on (after attach;
    // NVS must be initialized)
    void loadSaved();
    // Call from the LVGL task once a second
    void update();

    // Queue a change from another task (any task)
    void post(const UiCommand &cmd);
    // The study circle's friends, from the circle task (any task). Only the latest list is kept.
    void postFriends(const Friend *friends, int count);

    // ---- Actions (LVGL task) ----
    bool startSession();                    // with the current lengths and test timing
    bool startSession(int study, int brk, bool test = false);
    void endSessionEarly();
    void startNextSession();                // during a break or when READY: the set's next session
    void endSet();                          // the whole set -> idle
    void peek();                            // light the timer for 5 s
    void shh();                             // the pet hushes for 2 s
    void petted();                          // outside study, the pet is pleased for 2 s
    void setStudyLength(int study_min);     // resets the break to 1/5 of it
    void setBreakLength(int break_min);
    void setPet(int body, int face, int finish);
    void setFriends(const Friend *friends, int count);

    const Friend *friends(int *count) const { *count = friend_count; return friend_list; }
    const SessionResult &lastResult() const { return last_result; }
    const SessionLog &log() const { return session_log; }
    // Today as a SessionLog day number, or 0 while the clock isn't set
    int today() const;
    int sessionsToday() const { return logToday(session_log, today()); }

private:
    AppState();
    static void drain(lv_timer_t *t);
    void applySnapshot(const SessionSnapshot &snap);
    void handleEvent(const SessionEvent &event);
    void applyCommand(const UiCommand &cmd);
    void refreshMood();
    void recordSession(const SessionResult &result, bool completed);
    SavedState currentState();
    void scheduleSave();
    void saveNow();

    Friend friend_list[MAX_FRIENDS] = {};
    int friend_count = 0;
    SessionResult last_result = {};
    SessionLog session_log = {};
    QueueHandle_t commands = nullptr;
    QueueHandle_t friend_updates = nullptr;    // length 1, overwritten
    lv_timer_t *peek_timer = nullptr;
    lv_timer_t *shh_timer = nullptr;
    lv_timer_t *petted_timer = nullptr;
    lv_timer_t *save_timer = nullptr;
    bool persisting = false;
    bool shh_active = false;
    bool petted_active = false;
};
