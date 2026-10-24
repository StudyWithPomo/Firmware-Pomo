#include "app_state.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <system/system.hpp>
#include <app/session.hpp>
#include <ui/navigator.hpp>
#include <ui/pet/pet_data.h>
#include <comms/link.hpp>

#include "esp_log.h"

static const char *TAG = "state";

static const uint32_t PEEK_MS = 5000;
static const uint32_t SHH_MS = 2000;
static const uint32_t DRAIN_MS = 50;
// Save this long after the last change, so turning through Workshop options is one write
static const uint32_t SAVE_DELAY_MS = 2000;

AppState& AppState::instance() {
    static AppState inst;
    return inst;
}

static long current_time_cache = 0;

static char user_name_buf[17], user_name_prev[17];
static char pet_name_buf[11], pet_name_prev[11];
static char circle_names_buf[64], circle_names_prev[64];
static char drift_label_buf[16], drift_label_prev[16];

AppState::AppState() {
    current_time_cache = SystemAPI::instance().getCurrentTime();
    lv_subject_init_pointer(&time_subject, &current_time_cache);
    lv_subject_init_string(&user_name, user_name_buf, user_name_prev, sizeof(user_name_buf), "");

    lv_subject_init_string(&pet_name, pet_name_buf, pet_name_prev, sizeof(pet_name_buf), "Mochi");
    lv_subject_init_int(&pet_body, PET_BODY_BOT);
    lv_subject_init_int(&pet_face, PET_FACE_PLAIN);
    lv_subject_init_int(&pet_finish, PET_FINISH_BRASS);
    lv_subject_init_int(&pet_mood, PET_MOOD_HAPPY);

    lv_subject_init_int(&xp, 0);
    lv_subject_init_int(&level, 1);

    lv_subject_init_int(&multiplier, 1);
    lv_subject_init_int(&base_multiplier, 1);
    lv_subject_init_string(&circle_names, circle_names_buf, circle_names_prev, sizeof(circle_names_buf), "");
    lv_subject_init_int(&circle_version, 0);

    lv_subject_init_int(&study_min, 25);
    lv_subject_init_int(&break_min, 5);
    lv_subject_init_int(&always_show_timer, 0);

    lv_subject_init_int(&phase, (int)Phase::IDLE);
    lv_subject_init_int(&session_index, 1);
    lv_subject_init_int(&timer_remaining_s, 0);
    lv_subject_init_int(&timer_total_s, 0);
    lv_subject_init_int(&focus_state, 0);
    lv_subject_init_string(&drift_label, drift_label_buf, drift_label_prev, sizeof(drift_label_buf), "");
    lv_subject_init_int(&drift_seconds, 0);
    lv_subject_init_int(&peek_active, 0);
    lv_subject_init_int(&link_connected, 0);
    lv_subject_init_int(&log_version, 0);
}

void AppState::attach() {
    commands = xQueueCreate(8, sizeof(UiCommand));
    lv_timer_create(drain, DRAIN_MS, nullptr);
}

// ---- Saved progress and settings ----

static int clamp_int(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

void AppState::loadSaved() {
    SavedState s;
    storage_load(&s);

    // Flash could hold anything (an older firmware, a bad write): keep it in range
    if (s.pet_name[0]) lv_subject_copy_string(&pet_name, s.pet_name);
    lv_subject_copy_string(&user_name, s.user_name);
    setPet(clamp_int(s.pet_body, 0, PET_BODY_COUNT - 1), clamp_int(s.pet_face, 0, PET_FACE_COUNT - 1),
           clamp_int(s.pet_finish, 0, PET_FINISH_COUNT - 1));
    const int study = SessionCore::snapStudy(s.study_min);
    lv_subject_set_int(&study_min, study);
    lv_subject_set_int(&break_min, SessionCore::clampBreak(study, s.break_min));
    lv_subject_set_int(&always_show_timer, s.always_show_timer ? 1 : 0);
    Session::instance().setXp(s.xp < 0 ? 0 : s.xp);
    lv_subject_set_int(&xp, s.xp < 0 ? 0 : s.xp);
    session_log = s.log;
    lv_subject_set_int(&log_version, lv_subject_get_int(&log_version) + 1);

    // From now on, any change to a saved value is written shortly after
    persisting = true;
    lv_subject_t *saved[] = {&pet_name, &pet_body, &pet_face, &pet_finish, &user_name,
                             &study_min, &break_min, &always_show_timer, &xp};
    for (lv_subject_t *subject : saved) {
        lv_subject_add_observer(subject, [](lv_observer_t *, lv_subject_t *) { instance().scheduleSave(); }, nullptr);
    }
}

SavedState AppState::currentState() {
    SavedState s;
    storage_defaults(&s);
    s.xp = lv_subject_get_int(&xp);
    s.pet_body = lv_subject_get_int(&pet_body);
    s.pet_face = lv_subject_get_int(&pet_face);
    s.pet_finish = lv_subject_get_int(&pet_finish);
    strlcpy(s.pet_name, lv_subject_get_string(&pet_name), sizeof(s.pet_name));
    strlcpy(s.user_name, lv_subject_get_string(&user_name), sizeof(s.user_name));
    s.study_min = lv_subject_get_int(&study_min);
    s.break_min = lv_subject_get_int(&break_min);
    s.always_show_timer = lv_subject_get_int(&always_show_timer) != 0;
    s.log = session_log;
    return s;
}

void AppState::scheduleSave() {
    if (!persisting) return;
    if (!save_timer) {
        save_timer = lv_timer_create([](lv_timer_t *t) {
            lv_timer_pause(t);
            instance().saveNow();
        }, SAVE_DELAY_MS, nullptr);
    }
    lv_timer_reset(save_timer);
    lv_timer_resume(save_timer);
}

void AppState::saveNow() {
    if (!persisting) return;
    if (save_timer) lv_timer_pause(save_timer);
    storage_save(currentState());
}

void AppState::post(const UiCommand &cmd) {
    if (commands && xQueueSend(commands, &cmd, 0) != pdTRUE) ESP_LOGW(TAG, "UI command queue full");
}

// POSIX TZ strings count hours west of UTC, so +60 east (UTC+1) is "UTC-01:00"
static void set_timezone(int minutes_east) {
    char tz[16];
    const int west = -minutes_east;
    snprintf(tz, sizeof(tz), "UTC%c%02d:%02d", west < 0 ? '-' : '+', abs(west) / 60, abs(west) % 60);
    setenv("TZ", tz, 1);
    tzset();
}

void AppState::applyCommand(const UiCommand &cmd) {
    switch (cmd.type) {
        case UiCommand::CONFIG:
            if (cmd.has_user) lv_subject_copy_string(&user_name, cmd.user);
            if (cmd.has_pet_name) lv_subject_copy_string(&pet_name, cmd.pet_name);
            setPet(cmd.body >= 0 ? cmd.body : lv_subject_get_int(&pet_body),
                   cmd.face >= 0 ? cmd.face : lv_subject_get_int(&pet_face),
                   cmd.finish >= 0 ? cmd.finish : lv_subject_get_int(&pet_finish));
            if (cmd.has_tz) set_timezone(cmd.tz_minutes);
            if (cmd.has_time) SystemAPI::instance().setCurrentTime(cmd.epoch);
            if (cmd.has_time || cmd.has_tz) update();
            break;
        case UiCommand::PEEK:
            if ((Phase)lv_subject_get_int(&phase) == Phase::STUDY) peek();
            break;
        case UiCommand::LINK_SEEN:
            lv_subject_set_int(&link_connected, 1);
            break;
    }
}

void AppState::update() {
    const long current_time = SystemAPI::instance().getCurrentTime();
    if (current_time != current_time_cache) {
        current_time_cache = current_time;
        lv_subject_set_pointer(&time_subject, &current_time_cache);
    }
}

// ---- Session queues -> subjects (runs in the LVGL task) ----

void AppState::drain(lv_timer_t *t) {
    AppState &s = instance();
    Session &session = Session::instance();

    SessionSnapshot snap;
    if (xQueueReceive(session.snapshots(), &snap, 0) == pdTRUE) s.applySnapshot(snap);

    SessionEvent event;
    while (xQueueReceive(session.events(), &event, 0) == pdTRUE) s.handleEvent(event);

    UiCommand cmd;
    while (s.commands && xQueueReceive(s.commands, &cmd, 0) == pdTRUE) s.applyCommand(cmd);
}

void AppState::applySnapshot(const SessionSnapshot &snap) {
    lv_subject_set_int(&phase, (int)snap.phase);
    lv_subject_set_int(&session_index, snap.session_index);
    lv_subject_set_int(&timer_total_s, snap.total_s);
    lv_subject_set_int(&timer_remaining_s, snap.remaining_s);
    if (strcmp(lv_subject_get_string(&drift_label), snap.drift_label) != 0)
        lv_subject_copy_string(&drift_label, snap.drift_label);
    lv_subject_set_int(&drift_seconds, snap.drift_s);
    lv_subject_set_int(&focus_state, snap.drifting ? 1 : 0);
    lv_subject_set_int(&multiplier, snap.multiplier);
    lv_subject_set_int(&base_multiplier, snap.base_multiplier);
    lv_subject_set_int(&xp, snap.xp);
    lv_subject_set_int(&level, snap.level);
    refreshMood();
}

void AppState::handleEvent(const SessionEvent &event) {
    Navigator &nav = Navigator::instance();
    lv_subject_set_int(&peek_active, 0);

    switch (event.type) {
        case SessionEventType::STARTED:
        case SessionEventType::BREAK_ENDED:
            nav.reset(ScreenID::FOCUS);
            break;
        case SessionEventType::STUDY_ENDED:
            last_result = event.result;
            lv_subject_set_int(&xp, event.result.xp);
            recordSession(event.result, true);
            saveNow(); // progress is saved the moment a session is scored
            nav.reset(ScreenID::INTERMISSION);
            break;
        case SessionEventType::ENDED_EARLY:
            last_result = event.result;
            lv_subject_set_int(&xp, event.result.xp);
            recordSession(event.result, false);
            saveNow();
            nav.reset(ScreenID::HOME);
            break;
        case SessionEventType::SET_ENDED:
            nav.reset(ScreenID::HOME);
            break;
    }
}

void AppState::recordSession(const SessionResult &result, bool completed) {
    logSession(&session_log, result, completed, today());
    lv_subject_set_int(&log_version, lv_subject_get_int(&log_version) + 1);
    ESP_LOGI(TAG, "Log: %d sessions, %d sets, %d focused min, %d today", session_log.sessions,
             session_log.sets, session_log.focused_min, sessionsToday());
}

int AppState::today() const {
    if (current_time_cache < CLOCK_SET_AFTER) return 0;
    time_t t = (time_t)current_time_cache;
    struct tm tm;
    localtime_r(&t, &tm);
    return (tm.tm_year + 1900) * 1000 + tm.tm_yday + 1; // e.g. 2026297: same all day, never 0
}

void AppState::refreshMood() {
    PetMood mood = PET_MOOD_HAPPY;
    switch ((Phase)lv_subject_get_int(&phase)) {
        case Phase::STUDY:
            if (lv_subject_get_int(&focus_state)) mood = PET_MOOD_ANNOYED;
            else mood = shh_active ? PET_MOOD_SHH : PET_MOOD_FOCUS;
            break;
        case Phase::BREAK:
        case Phase::LONG_REST:
            mood = PET_MOOD_LOVE; // "wound up"
            break;
        default:
            if (petted_active) mood = PET_MOOD_LOVE;
            break;
    }
    lv_subject_set_int(&pet_mood, mood);
}

// ---- Actions ----

bool AppState::startSession() {
    return startSession(lv_subject_get_int(&study_min), lv_subject_get_int(&break_min));
}

bool AppState::startSession(int study, int brk) {
    return Session::instance().start(study, brk);
}

void AppState::endSessionEarly() {
    Session::instance().endEarly();
}

void AppState::peek() {
    lv_subject_set_int(&peek_active, 1);
    if (!peek_timer) {
        peek_timer = lv_timer_create([](lv_timer_t *t) {
            AppState &s = instance();
            lv_subject_set_int(&s.peek_active, 0);
            lv_timer_pause(t);
        }, PEEK_MS, nullptr);
    }
    lv_timer_reset(peek_timer);
    lv_timer_resume(peek_timer);
}

void AppState::shh() {
    shh_active = true;
    refreshMood();
    if (!shh_timer) {
        shh_timer = lv_timer_create([](lv_timer_t *t) {
            AppState &s = instance();
            s.shh_active = false;
            s.refreshMood();
            lv_timer_pause(t);
        }, SHH_MS, nullptr);
    }
    lv_timer_reset(shh_timer);
    lv_timer_resume(shh_timer);
}

void AppState::petted() {
    petted_active = true;
    refreshMood();
    if (!petted_timer) {
        petted_timer = lv_timer_create([](lv_timer_t *t) {
            AppState &s = instance();
            s.petted_active = false;
            s.refreshMood();
            lv_timer_pause(t);
        }, SHH_MS, nullptr);
    }
    lv_timer_reset(petted_timer);
    lv_timer_resume(petted_timer);
}

void AppState::setStudyLength(int study) {
    study = SessionCore::snapStudy(study);
    lv_subject_set_int(&study_min, study);
    // The break resets to the classic 1/5 (25 -> 5, 45 -> 9), so turning through 45 and
    // back to 25 lands on 25 / 5 again
    lv_subject_set_int(&break_min, SessionCore::breakMin(study));
}

void AppState::setBreakLength(int brk) {
    lv_subject_set_int(&break_min, SessionCore::clampBreak(lv_subject_get_int(&study_min), brk));
}

void AppState::setPet(int body, int face, int finish) {
    lv_subject_set_int(&pet_body, body);
    lv_subject_set_int(&pet_face, face);
    lv_subject_set_int(&pet_finish, finish);
}

void AppState::setFriends(const Friend *list, int count) {
    if (count > MAX_FRIENDS) count = MAX_FRIENDS;
    memcpy(friend_list, list, sizeof(Friend) * count);
    friend_count = count;

    char names[sizeof(circle_names_buf)] = "";
    for (int i = 0; i < count; i++) {
        if (i) strlcat(names, ", ", sizeof(names));
        strlcat(names, friend_list[i].name, sizeof(names));
    }
    if (strcmp(lv_subject_get_string(&circle_names), names) != 0) lv_subject_copy_string(&circle_names, names);
    lv_subject_set_int(&circle_version, lv_subject_get_int(&circle_version) + 1);
    link_send_circle();
    ESP_LOGI(TAG, "Circle: %d friend(s)", count);
}
