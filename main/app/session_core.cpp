#include "session_core.hpp"

#include <string.h>

#include "utilities.h"

constexpr int SessionCore::STUDY_OPTIONS[];

int SessionCore::snapStudy(int study_min)
{
    int best = STUDY_OPTIONS[0];
    for (int i = 1; i < STUDY_OPTION_COUNT; i++) {
        const int d = STUDY_OPTIONS[i] - study_min;
        const int best_d = best - study_min;
        if ((d < 0 ? -d : d) < (best_d < 0 ? -best_d : best_d)) best = STUDY_OPTIONS[i];
    }
    return best;
}

int SessionCore::clampBreak(int study_min, int break_min)
{
    const int lo = breakMin(study_min);
    const int hi = breakMax(study_min);
    return break_min < lo ? lo : (break_min > hi ? hi : break_min);
}

int SessionCore::baseMultiplier() const
{
    const int m = 1 + circle_friends_;
    return m > MAX_MULTIPLIER ? MAX_MULTIPLIER : m;
}

int SessionCore::effectiveMultiplier() const
{
    // The bonus pauses while anyone in the circle, us included, is drifting
    if (drifting() || friend_drifting_) return 1;
    return baseMultiplier();
}

bool SessionCore::start(int study_min, int break_min)
{
    if (phase_ != Phase::IDLE) return false;
    study_min_ = snapStudy(study_min);
    break_min_ = clampBreak(study_min_, break_min);
    beginStudy();
    return true;
}

void SessionCore::beginStudy()
{
    phase_ = Phase::STUDY;
    total_s_ = remaining_s_ = study_min_ * 60;
    minute_s_ = 0;
    minute_drift_s_ = 0;
    minute_mult_ = MAX_MULTIPLIER;  // lowered by every second of the minute in tick()
    focused_min_ = 0;
    elapsed_min_ = 0;
    earned_xp_ = 0;
    resetCamera();
}

void SessionCore::goIdle()
{
    phase_ = Phase::IDLE;
    index_ = 1;
    remaining_s_ = total_s_ = 0;
    resetCamera();
}

void SessionCore::closeMinute()
{
    elapsed_min_++;
    if (minute_drift_s_ < DRIFT_MINUTE_S) {
        focused_min_++;
        earned_xp_ += minute_mult_;
    }
    minute_s_ = 0;
    minute_drift_s_ = 0;
    minute_mult_ = MAX_MULTIPLIER;
}

SessionResult SessionCore::finishStudy()
{
    SessionResult r = {};
    r.focused_min = focused_min_;
    r.elapsed_min = elapsed_min_;
    r.pct = elapsed_min_ > 0 ? focused_min_ * 100 / elapsed_min_ : 0;

    // Nothing to score if not even a minute passed
    if (elapsed_min_ > 0) {
        if (r.pct < PENALTY_PCT) {
            // No XP, -10, but never below the start of the current level
            const int floor = (levelFor(xp_) - 1) * XP_PER_LEVEL;
            const int next = xp_ - PENALTY_XP < floor ? floor : xp_ - PENALTY_XP;
            r.penalty = true;
            r.xp_delta = next - xp_;
            xp_ = next;
        } else {
            r.xp_delta = earned_xp_;
            xp_ += earned_xp_;
        }
    }
    r.xp = xp_;
    r.level = levelFor(xp_);
    r.session_index = index_;
    return r;
}

void logSession(SessionLog *log, const SessionResult &result, bool completed, int day)
{
    log->focused_min += result.focused_min;
    if (!completed) return;
    log->sessions++;
    if (result.session_index >= SessionCore::SESSIONS_PER_SET) log->sets++;
    if (day == 0) return;
    if (log->today_day != day) {
        log->today_day = day;
        log->today = 0;
    }
    log->today++;
}

int logToday(const SessionLog &log, int day)
{
    return day != 0 && log.today_day == day ? log.today : 0;
}

bool SessionCore::endEarly(SessionEvent *out)
{
    if (phase_ != Phase::STUDY) return false;
    out->type = SessionEventType::ENDED_EARLY;
    out->result = finishStudy();   // a partial last minute isn't counted
    goIdle();
    return true;
}

bool SessionCore::endBreak(SessionEvent *out)
{
    out->result = {};
    switch (phase_) {
        case Phase::BREAK:
            index_++;
            beginStudy();
            out->type = SessionEventType::BREAK_ENDED;
            return true;
        case Phase::LONG_REST:
            goIdle();
            out->type = SessionEventType::SET_ENDED;
            return true;
        default:
            return false;
    }
}

bool SessionCore::endSet(SessionEvent *out)
{
    switch (phase_) {
        case Phase::STUDY:
            return endEarly(out);
        case Phase::BREAK:
        case Phase::LONG_REST:
            goIdle();   // nothing to score outside study
            out->type = SessionEventType::SET_ENDED;
            out->result = {};
            return true;
        default:
            return false;
    }
}

bool SessionCore::tick(SessionEvent *out)
{
    if (phase_ == Phase::IDLE) return false;

    if (phase_ == Phase::STUDY) tickCamera();

    if (drifting()) drift_s_++;

    if (phase_ == Phase::STUDY) {
        if (drifting()) minute_drift_s_++;
        const int m = effectiveMultiplier();
        if (m < minute_mult_) minute_mult_ = m;
        if (++minute_s_ >= 60) closeMinute();
    }

    if (--remaining_s_ > 0) return false;

    switch (phase_) {
        case Phase::STUDY:
            out->type = SessionEventType::STUDY_ENDED;
            out->result = finishStudy();
            resetCamera();  // camera drift only counts during study
            phase_ = index_ >= SESSIONS_PER_SET ? Phase::LONG_REST : Phase::BREAK;
            total_s_ = remaining_s_ = (phase_ == Phase::LONG_REST ? LONG_REST_MIN : break_min_) * 60;
            return true;

        case Phase::BREAK:
        case Phase::LONG_REST:
            return endBreak(out);

        default:
            return false;
    }
}

void SessionCore::driftChanged(bool was)
{
    const bool now = drifting();
    if (now && !was) {
        drift_s_ = 0;
        minute_mult_ = 1;
    } else if (!now) {
        drift_s_ = 0;
    }
}

void SessionCore::setDrift(bool active, const char *label, DriftKind kind)
{
    const bool was = drifting();
    laptop_drift_ = active;
    if (active) {
        laptop_kind_ = kind == DriftKind::IDLE ? DriftKind::IDLE : DriftKind::SITE;
        copy_utf8(laptop_label_, sizeof(laptop_label_), label);
    } else {
        laptop_label_[0] = '\0';
    }
    driftChanged(was);
}

void SessionCore::setFocus(CameraState state, const char *label)
{
    if (phase_ != Phase::STUDY) return;
    camera_heard_ = true;
    camera_silent_s_ = 0;

    CameraRun run = CameraRun::NONE;   // calibrating neither starts nor ends a drift
    if (state == CameraState::FOCUSED) run = CameraRun::GOOD;
    else if (state == CameraState::DRIFTING || state == CameraState::UNFOCUSED) run = CameraRun::BAD;
    if (run != camera_run_) {
        camera_run_ = run;
        camera_run_s_ = 0;
    }
    if (run == CameraRun::BAD) {
        copy_utf8(camera_reason_, sizeof(camera_reason_), label);
        if (camera_drift_) memcpy(camera_label_, camera_reason_, sizeof(camera_label_));
    }
}

void SessionCore::tickCamera()
{
    if (!camera_heard_) return;
    // Counts timer seconds since the last tick, so it is already 1 one second after a tick
    if (++camera_silent_s_ > CAMERA_TIMEOUT_S) {
        resetCamera();  // the focus server stopped
        return;
    }
    camera_run_s_++;
    if (camera_run_ == CameraRun::BAD && camera_run_s_ >= CAMERA_DRIFT_S) setCameraDrift(true);
    else if (camera_run_ == CameraRun::GOOD && camera_run_s_ >= CAMERA_CLEAR_S) setCameraDrift(false);
}

void SessionCore::setCameraDrift(bool active)
{
    const bool was = drifting();
    camera_drift_ = active;
    if (active) memcpy(camera_label_, camera_reason_, sizeof(camera_label_));
    else camera_label_[0] = '\0';
    driftChanged(was);
}

void SessionCore::resetCamera()
{
    setCameraDrift(false);
    camera_run_ = CameraRun::NONE;
    camera_run_s_ = 0;
    camera_heard_ = false;
    camera_silent_s_ = 0;
    camera_reason_[0] = '\0';
}

void SessionCore::setCircle(int friends_in_session, bool friend_drifting)
{
    circle_friends_ = friends_in_session < 0 ? 0 : friends_in_session;
    friend_drifting_ = friend_drifting;
    const int m = effectiveMultiplier();
    if (m < minute_mult_) minute_mult_ = m;
}

void SessionCore::snapshot(SessionSnapshot *out) const
{
    out->phase = phase_;
    out->session_index = index_;
    out->study_min = study_min_;
    out->break_min = break_min_;
    out->remaining_s = remaining_s_;
    out->total_s = total_s_;
    out->drifting = drifting();
    // The laptop's drift is shown when both sources are active
    if (laptop_drift_) {
        out->drift_kind = laptop_kind_;
        memcpy(out->drift_label, laptop_label_, sizeof(out->drift_label));
    } else if (camera_drift_) {
        out->drift_kind = DriftKind::CAMERA;
        memcpy(out->drift_label, camera_label_, sizeof(out->drift_label));
    } else {
        out->drift_kind = DriftKind::NONE;
        out->drift_label[0] = '\0';
    }
    out->drift_s = drift_s_;
    out->multiplier = effectiveMultiplier();
    out->base_multiplier = baseMultiplier();
    out->xp = xp_;
    out->level = levelFor(xp_);
}
