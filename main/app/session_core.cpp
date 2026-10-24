#include "session_core.hpp"

#include <string.h>

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
    if (drifting_ || friend_drifting_) return 1;
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
    return r;
}

bool SessionCore::endEarly(SessionEvent *out)
{
    if (phase_ != Phase::STUDY) return false;
    out->type = SessionEventType::ENDED_EARLY;
    out->result = finishStudy();   // a partial last minute isn't counted
    phase_ = Phase::IDLE;
    index_ = 1;
    remaining_s_ = total_s_ = 0;
    return true;
}

bool SessionCore::tick(SessionEvent *out)
{
    if (phase_ == Phase::IDLE) return false;

    if (drifting_) drift_s_++;

    if (phase_ == Phase::STUDY) {
        if (drifting_) minute_drift_s_++;
        const int m = effectiveMultiplier();
        if (m < minute_mult_) minute_mult_ = m;
        if (++minute_s_ >= 60) closeMinute();
    }

    if (--remaining_s_ > 0) return false;

    switch (phase_) {
        case Phase::STUDY:
            out->type = SessionEventType::STUDY_ENDED;
            out->result = finishStudy();
            phase_ = index_ >= SESSIONS_PER_SET ? Phase::LONG_REST : Phase::BREAK;
            total_s_ = remaining_s_ = (phase_ == Phase::LONG_REST ? LONG_REST_MIN : break_min_) * 60;
            return true;

        case Phase::BREAK:
            index_++;
            beginStudy();
            out->type = SessionEventType::BREAK_ENDED;
            out->result = {};
            return true;

        case Phase::LONG_REST:
            phase_ = Phase::IDLE;
            index_ = 1;
            remaining_s_ = total_s_ = 0;
            out->type = SessionEventType::SET_ENDED;
            out->result = {};
            return true;

        default:
            return false;
    }
}

void SessionCore::setDrift(bool active, const char *label)
{
    if (active) {
        if (!drifting_) drift_s_ = 0;
        drifting_ = true;
        strncpy(drift_label_, label ? label : "", sizeof(drift_label_) - 1);
        drift_label_[sizeof(drift_label_) - 1] = '\0';
        minute_mult_ = 1;
    } else {
        drifting_ = false;
        drift_s_ = 0;
        drift_label_[0] = '\0';
    }
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
    out->drifting = drifting_;
    memcpy(out->drift_label, drift_label_, sizeof(out->drift_label));
    out->drift_s = drift_s_;
    out->multiplier = effectiveMultiplier();
    out->base_multiplier = baseMultiplier();
    out->xp = xp_;
    out->level = levelFor(xp_);
}
