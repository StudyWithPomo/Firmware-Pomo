#pragma once
// Study session rules. Plain C++ with no ESP-IDF or LVGL dependencies, so it can be
// tested on a PC. Session (session.hpp) drives it from a 1 s timer and handles threading.
#include <stdint.h>

enum class Phase : uint8_t { IDLE, STUDY, BREAK, LONG_REST };

// Where the drift shown comes from: the laptop (site, idle) or the camera ticks
enum class DriftKind : uint8_t { NONE, SITE, IDLE, CAMERA };

// Camera focus state from ML-Server-Pomo, one per focus tick
enum class CameraState : uint8_t { CALIBRATING, FOCUSED, DRIFTING, UNFOCUSED };

enum class SessionEventType : uint8_t {
    STARTED,        // a study session began
    STUDY_ENDED,    // study time ran out; a break or long rest began (result is set)
    ENDED_EARLY,    // the user ended the session mid-study; back to idle (result is set)
    BREAK_ENDED,    // the break ran out or was ended, and the next study session began
    SET_ENDED,      // the long rest ran out, or the set was ended outside study; back to idle
};

struct SessionResult {
    int pct;            // focused minutes / elapsed minutes, 0-100
    int focused_min;
    int elapsed_min;
    int xp_delta;       // XP gained (or lost to the penalty) this session
    bool penalty;       // under 50% focus
    int xp;             // total XP afterwards
    int level;
};

struct SessionEvent {
    SessionEventType type;
    SessionResult result;
};

struct SessionSnapshot {
    Phase phase;
    int session_index;      // 1-4
    int study_min;
    int break_min;
    int remaining_s;        // in the current phase
    int total_s;            // length of the current phase
    bool drifting;          // any source
    DriftKind drift_kind;   // source of the label shown (laptop wins), NONE when not drifting
    char drift_label[16];
    int drift_s;            // how long the device has been drifting, any source
    int multiplier;         // effective right now (1 while anyone drifts)
    int base_multiplier;    // what the circle earns when nobody is drifting
    int xp;
    int level;
};

class SessionCore {
public:
    static constexpr int SESSIONS_PER_SET = 4;
    static constexpr int LONG_REST_MIN = 15;
    static constexpr int XP_PER_LEVEL = 500;
    static constexpr int DRIFT_MINUTE_S = 15;   // a minute with 15+ s of drift is a drift minute
    static constexpr int PENALTY_PCT = 50;      // under this, no XP and -10
    static constexpr int PENALTY_XP = 10;
    static constexpr int MAX_MULTIPLIER = 3;
    // Camera drift, from focus ticks during study
    static constexpr int CAMERA_DRIFT_S = 10;   // continuous drifting/unfocused to start
    static constexpr int CAMERA_CLEAR_S = 3;    // continuous focused to end
    static constexpr int CAMERA_TIMEOUT_S = 5;  // no tick for this long ends it

    // Study lengths offered by the UI
    static constexpr int STUDY_OPTIONS[] = {15, 25, 45, 60};
    static constexpr int STUDY_OPTION_COUNT = 4;

    // Break must be 1/5 to 1/3 of study time: 15 -> 3-5, 25 -> 5-8, 45 -> 9-15, 60 -> 12-20
    static int breakMin(int study_min) { return (study_min + 4) / 5; }
    static int breakMax(int study_min) { return study_min / 3; }
    static int clampBreak(int study_min, int break_min);
    // Nearest allowed study length
    static int snapStudy(int study_min);
    static int levelFor(int xp) { return xp / XP_PER_LEVEL + 1; }

    // Starts session 1 of a set (or the next one, if mid-set). Returns false if a phase is
    // already running. Lengths are snapped to the allowed values.
    bool start(int study_min, int break_min);
    // Ends the current study session now and applies the XP rules. Returns false if not studying.
    bool endEarly(SessionEvent *out);
    // Ends the break (next study session starts) or the long rest (back to idle).
    // Returns false outside a break or long rest.
    bool endBreak(SessionEvent *out);
    // Ends the whole set: scores the session if studying (as endEarly), then idle.
    // Returns false if idle.
    bool endSet(SessionEvent *out);
    // Advance one second. Returns true and fills `out` when something happened.
    bool tick(SessionEvent *out);

    // Laptop drift (site or idle). The camera drift is separate; either one means drifting.
    void setDrift(bool active, const char *label, DriftKind kind = DriftKind::SITE);
    // One camera focus tick; `label` is what to show if it turns into a drift. Ignored
    // outside study. tick() decides when the camera drift starts and ends.
    void setFocus(CameraState state, const char *label);
    // From the study circle: friends in session (so multiplier = 1 + friends, capped) and
    // whether any of them is drifting right now
    void setCircle(int friends_in_session, bool friend_drifting);
    void setXp(int xp) { xp_ = xp < 0 ? 0 : xp; }

    void snapshot(SessionSnapshot *out) const;
    Phase phase() const { return phase_; }
    int xp() const { return xp_; }

private:
    int baseMultiplier() const;
    int effectiveMultiplier() const;
    void beginStudy();
    void goIdle();
    void closeMinute();
    SessionResult finishStudy();
    bool drifting() const { return laptop_drift_ || camera_drift_; }
    // Call after either drift source may have changed; `was` is drifting() before
    void driftChanged(bool was);
    void setCameraDrift(bool active);
    void resetCamera();
    void tickCamera();

    Phase phase_ = Phase::IDLE;
    int index_ = 1;
    int study_min_ = 25;
    int break_min_ = 5;
    int remaining_s_ = 0;
    int total_s_ = 0;

    // Current study session
    int minute_s_ = 0;          // seconds into the current minute
    int minute_drift_s_ = 0;
    int minute_mult_ = 1;       // lowest multiplier during any second of this minute
    int focused_min_ = 0;
    int elapsed_min_ = 0;
    int earned_xp_ = 0;

    bool laptop_drift_ = false;
    DriftKind laptop_kind_ = DriftKind::SITE;
    char laptop_label_[16] = {0};
    bool camera_drift_ = false;
    char camera_label_[16] = {0};
    int drift_s_ = 0;

    // Camera ticks: the state class of the current run and how long it has lasted
    enum class CameraRun : uint8_t { NONE, GOOD, BAD };
    CameraRun camera_run_ = CameraRun::NONE;
    int camera_run_s_ = 0;
    bool camera_heard_ = false;     // a tick arrived this study session
    int camera_silent_s_ = 0;       // since the last tick
    char camera_reason_[16] = {0};  // label from the latest tick

    int circle_friends_ = 0;
    bool friend_drifting_ = false;

    int xp_ = 0;
};
