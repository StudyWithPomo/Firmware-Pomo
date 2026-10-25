// Host test for the study session rules (main/app/session_core.cpp). No ESP-IDF needed:
//   g++ -std=c++17 -Wall -I main test/session_test.cpp main/app/session_core.cpp -o session_test && ./session_test

#include <stdio.h>
#include <string.h>

#include "app/session_core.hpp"
#include "utilities.h"

static int failures = 0;

#define CHECK_EQ(actual, expected)                                                        \
    do {                                                                                  \
        const long a_ = (long)(actual), e_ = (long)(expected);                            \
        if (a_ != e_) {                                                                   \
            printf("  FAIL %s:%d  %s == %ld, expected %ld\n", __FILE__, __LINE__, #actual, a_, e_); \
            failures++;                                                                   \
        }                                                                                 \
    } while (0)

#define CHECK_STR(actual, expected)                                                       \
    do {                                                                                  \
        if (strcmp((actual), (expected)) != 0) {                                          \
            printf("  FAIL %s:%d  %s == \"%s\", expected \"%s\"\n", __FILE__, __LINE__,   \
                   #actual, (actual), (expected));                                        \
            failures++;                                                                   \
        }                                                                                 \
    } while (0)

// Tick `seconds` times; returns the last event seen (type cast to int, -1 if none)
static int run(SessionCore &s, int seconds, SessionEvent *last = nullptr)
{
    int type = -1;
    SessionEvent e;
    for (int i = 0; i < seconds; i++) {
        if (s.tick(&e)) {
            type = (int)e.type;
            if (last) *last = e;
        }
    }
    return type;
}

static void test_break_ranges()
{
    puts("break ranges");
    CHECK_EQ(SessionCore::breakMin(15), 3); CHECK_EQ(SessionCore::breakMax(15), 5);
    CHECK_EQ(SessionCore::breakMin(25), 5); CHECK_EQ(SessionCore::breakMax(25), 8);
    CHECK_EQ(SessionCore::breakMin(45), 9); CHECK_EQ(SessionCore::breakMax(45), 15);
    CHECK_EQ(SessionCore::breakMin(60), 12); CHECK_EQ(SessionCore::breakMax(60), 20);
    CHECK_EQ(SessionCore::clampBreak(25, 20), 8);
    CHECK_EQ(SessionCore::clampBreak(25, 1), 5);
    CHECK_EQ(SessionCore::breakMin(90), 18); CHECK_EQ(SessionCore::breakMax(90), 30);
    CHECK_EQ(SessionCore::breakMin(35), 7); CHECK_EQ(SessionCore::breakMax(35), 11);
}

static void test_study_lengths()
{
    puts("study lengths: 15-90 in steps of 5");
    CHECK_EQ(SessionCore::snapStudy(30), 30);
    CHECK_EQ(SessionCore::snapStudy(35), 35);
    CHECK_EQ(SessionCore::snapStudy(32), 30);
    CHECK_EQ(SessionCore::snapStudy(33), 35);
    CHECK_EQ(SessionCore::snapStudy(10), 15);
    CHECK_EQ(SessionCore::snapStudy(-5), 15);
    CHECK_EQ(SessionCore::snapStudy(100), 90);
    CHECK_EQ(SessionCore::snapStudy(88), 90);
    CHECK_EQ(SessionCore::snapStudy(25), 25);
    // the long rest is never shorter than the longest allowed break
    CHECK_EQ(SessionCore::longRestMin(15), 15);
    CHECK_EQ(SessionCore::longRestMin(25), 15);
    CHECK_EQ(SessionCore::longRestMin(60), 20);
    CHECK_EQ(SessionCore::longRestMin(90), 30);

    SessionCore s;
    CHECK_EQ(s.start(35, 9), true);
    SessionSnapshot snap;
    s.snapshot(&snap);
    CHECK_EQ(snap.study_min, 35);
    CHECK_EQ(snap.break_min, 9);
    CHECK_EQ(snap.total_s, 35 * 60);
}

static void test_long_rest_for_long_sessions()
{
    puts("90 min sessions get a 30 min long rest");
    SessionCore s;
    s.start(90, 18);
    for (int i = 1; i <= 3; i++) {
        CHECK_EQ(run(s, 90 * 60), (int)SessionEventType::STUDY_ENDED);
        CHECK_EQ(run(s, 18 * 60), (int)SessionEventType::BREAK_OVER);
        SessionEvent e;
        CHECK_EQ(s.endBreak(&e), true);
    }
    CHECK_EQ(run(s, 90 * 60), (int)SessionEventType::STUDY_ENDED);
    CHECK_EQ((int)s.phase(), (int)Phase::LONG_REST);
    SessionSnapshot snap;
    s.snapshot(&snap);
    CHECK_EQ(snap.remaining_s, 30 * 60);
    CHECK_EQ(run(s, 30 * 60), (int)SessionEventType::SET_ENDED);
}

static void test_earned_in_snapshot()
{
    puts("earned XP shows during study, then resets");
    SessionCore s;
    s.start(15, 3);
    SessionSnapshot snap;
    s.snapshot(&snap);
    CHECK_EQ(snap.earned, 0);
    run(s, 3 * 60);                 // three focused minutes, no circle: 1 XP each
    s.snapshot(&snap);
    CHECK_EQ(snap.earned, 3);
    CHECK_EQ(snap.xp, 0);           // not scored until the session ends
    run(s, 12 * 60);                // session ends, break starts
    s.snapshot(&snap);
    CHECK_EQ((int)snap.phase, (int)Phase::BREAK);
    CHECK_EQ(snap.earned, 0);
    CHECK_EQ(snap.xp, 15);
}

static void test_full_session_and_break()
{
    puts("full 25/5 session, then break");
    SessionCore s;
    CHECK_EQ(s.start(25, 5), true);
    CHECK_EQ(s.start(25, 5), false); // already running
    SessionEvent e = {};
    CHECK_EQ(run(s, 25 * 60, &e), (int)SessionEventType::STUDY_ENDED);
    CHECK_EQ(e.result.pct, 100);
    CHECK_EQ(e.result.xp_delta, 25);
    CHECK_EQ(s.xp(), 25);
    CHECK_EQ((int)s.phase(), (int)Phase::BREAK);

    SessionSnapshot snap;
    s.snapshot(&snap);
    CHECK_EQ(snap.remaining_s, 5 * 60);

    // The break running out waits for the user
    CHECK_EQ(run(s, 5 * 60), (int)SessionEventType::BREAK_OVER);
    s.snapshot(&snap);
    CHECK_EQ((int)snap.phase, (int)Phase::READY);
    CHECK_EQ(snap.session_index, 1);
    CHECK_EQ(snap.remaining_s, 0);
    CHECK_EQ(run(s, 10 * 60), -1);              // and keeps waiting
    CHECK_EQ((int)s.phase(), (int)Phase::READY);

    CHECK_EQ(s.endBreak(&e), true);             // Start next
    CHECK_EQ((int)e.type, (int)SessionEventType::BREAK_ENDED);
    s.snapshot(&snap);
    CHECK_EQ((int)snap.phase, (int)Phase::STUDY);
    CHECK_EQ(snap.session_index, 2);
    CHECK_EQ(snap.remaining_s, 25 * 60);
}

static void test_drift_minutes()
{
    puts("drift minutes (15 s threshold)");
    SessionCore s;
    s.start(15, 3);
    // Minute 1: 14 s of drift -> still focused, but the bonus paused so it earns 1
    s.setDrift(true, "YouTube");
    run(s, 14);
    s.setDrift(false, nullptr);
    run(s, 46);
    // Minute 2: 15 s of drift -> drift minute
    s.setDrift(true, "YouTube");
    run(s, 15);
    s.setDrift(false, nullptr);
    run(s, 45);
    SessionEvent e = {};
    run(s, 13 * 60, &e);
    CHECK_EQ(e.result.elapsed_min, 15);
    CHECK_EQ(e.result.focused_min, 14);
    CHECK_EQ(e.result.xp_delta, 14);
}

static void test_multiplier()
{
    puts("circle multiplier and bonus pause");
    SessionCore s;
    s.setCircle(1, false); // one friend -> 2x
    s.start(15, 3);
    SessionSnapshot snap;
    s.snapshot(&snap);
    CHECK_EQ(snap.multiplier, 2);
    // Friend drifts through minute 1 -> that minute earns 1
    s.setCircle(1, true);
    s.snapshot(&snap);
    CHECK_EQ(snap.multiplier, 1);
    CHECK_EQ(snap.base_multiplier, 2);
    run(s, 60);
    s.setCircle(1, false);
    SessionEvent e = {};
    run(s, 14 * 60, &e);
    CHECK_EQ(e.result.xp_delta, 1 + 14 * 2);

    // Me, Dev and Ava -> 3x; me alone -> 1x
    SessionCore three;
    three.setCircle(2, false);
    three.snapshot(&snap);
    CHECK_EQ(snap.multiplier, 3);
    three.setCircle(0, false);
    three.snapshot(&snap);
    CHECK_EQ(snap.multiplier, 1);

    SessionCore capped;
    capped.setCircle(5, false);
    capped.snapshot(&snap);
    CHECK_EQ(snap.multiplier, 3);
}

static void test_penalty_floor()
{
    puts("under 50%: -10, never below the level start");
    {
        SessionCore s;
        s.setXp(505);               // Lv 2 starts at 500
        s.start(15, 3);
        s.setDrift(true, "Reddit"); // whole session drifting
        SessionEvent e = {};
        run(s, 15 * 60, &e);
        CHECK_EQ(e.result.pct, 0);
        CHECK_EQ(e.result.penalty, true);
        CHECK_EQ(e.result.xp_delta, -5);
        CHECK_EQ(s.xp(), 500);
        CHECK_EQ(e.result.level, 2);
    }
    {
        SessionCore s;
        s.setXp(600);
        s.start(15, 3);
        s.setDrift(true, "Reddit");
        SessionEvent e = {};
        run(s, 15 * 60, &e);
        CHECK_EQ(s.xp(), 590);
    }
}

static void test_set_and_long_rest()
{
    puts("four sessions, then the 15 min long rest");
    SessionCore s;
    s.start(15, 3);
    for (int i = 1; i <= 3; i++) {
        CHECK_EQ(run(s, 15 * 60), (int)SessionEventType::STUDY_ENDED);
        CHECK_EQ(run(s, 3 * 60), (int)SessionEventType::BREAK_OVER);
        SessionEvent e;
        CHECK_EQ(s.endBreak(&e), true);
    }
    CHECK_EQ(run(s, 15 * 60), (int)SessionEventType::STUDY_ENDED);
    CHECK_EQ((int)s.phase(), (int)Phase::LONG_REST);
    SessionSnapshot snap;
    s.snapshot(&snap);
    CHECK_EQ(snap.remaining_s, 15 * 60);
    CHECK_EQ(run(s, 15 * 60), (int)SessionEventType::SET_ENDED);
    s.snapshot(&snap);
    CHECK_EQ((int)snap.phase, (int)Phase::IDLE);
    CHECK_EQ(snap.session_index, 1);
    CHECK_EQ(s.xp(), 60);
}

static void test_end_early()
{
    puts("ending early");
    SessionCore s;
    s.start(25, 5);
    run(s, 10 * 60 + 30); // the half minute isn't counted
    SessionEvent e = {};
    CHECK_EQ(s.endEarly(&e), true);
    CHECK_EQ((int)e.type, (int)SessionEventType::ENDED_EARLY);
    CHECK_EQ(e.result.elapsed_min, 10);
    CHECK_EQ(e.result.xp_delta, 10);
    CHECK_EQ((int)s.phase(), (int)Phase::IDLE);
    CHECK_EQ(s.endEarly(&e), false);

    SessionCore quick;
    quick.setXp(42);
    quick.start(25, 5);
    run(quick, 30);
    quick.endEarly(&e);
    CHECK_EQ(e.result.xp_delta, 0); // under a minute: nothing scored, no penalty
    CHECK_EQ(quick.xp(), 42);
}

// Send a focus tick every half second for `seconds`, like ML-Server-Pomo at 2 Hz
static void focus_for(SessionCore &s, int seconds, CameraState state, const char *label = "Camera")
{
    SessionEvent e;
    for (int i = 0; i < seconds; i++) {
        s.setFocus(state, label);
        s.setFocus(state, label);
        s.tick(&e);
    }
}

static SessionSnapshot snap_of(const SessionCore &s)
{
    SessionSnapshot snap;
    s.snapshot(&snap);
    return snap;
}

static void test_camera_drift()
{
    puts("camera drift from focus ticks");
    SessionCore s;
    s.start(25, 5);
    focus_for(s, 9, CameraState::UNFOCUSED, "Looking away");
    CHECK_EQ(snap_of(s).drifting, false);       // 9 s is not sustained yet
    focus_for(s, 1, CameraState::DRIFTING, "Looking away");
    SessionSnapshot snap = snap_of(s);
    CHECK_EQ(snap.drifting, true);              // drifting counts the same as unfocused
    CHECK_EQ((int)snap.drift_kind, (int)DriftKind::CAMERA);
    CHECK_STR(snap.drift_label, "Looking away");
    CHECK_EQ(snap.multiplier, 1);

    focus_for(s, 2, CameraState::UNFOCUSED, "Away");
    CHECK_STR(snap_of(s).drift_label, "Away");  // label follows the reason

    focus_for(s, 2, CameraState::FOCUSED);
    CHECK_EQ(snap_of(s).drifting, true);        // 2 s focused is not enough
    focus_for(s, 1, CameraState::FOCUSED);
    snap = snap_of(s);
    CHECK_EQ(snap.drifting, false);
    CHECK_EQ((int)snap.drift_kind, (int)DriftKind::NONE);
    CHECK_STR(snap.drift_label, "");

    // A focused tick in the middle breaks the run
    focus_for(s, 8, CameraState::UNFOCUSED);
    focus_for(s, 1, CameraState::FOCUSED);
    focus_for(s, 8, CameraState::UNFOCUSED);
    CHECK_EQ(snap_of(s).drifting, false);

    // Calibrating neither starts nor ends a drift
    focus_for(s, 20, CameraState::CALIBRATING);
    CHECK_EQ(snap_of(s).drifting, false);
    focus_for(s, 10, CameraState::UNFOCUSED);
    CHECK_EQ(snap_of(s).drifting, true);
    focus_for(s, 20, CameraState::CALIBRATING);
    CHECK_EQ(snap_of(s).drifting, true);
}

static void test_camera_timeout_and_phases()
{
    puts("camera drift: server stops, outside study");
    SessionCore s;
    s.start(15, 3);
    focus_for(s, 10, CameraState::UNFOCUSED);
    CHECK_EQ(snap_of(s).drifting, true);
    run(s, 4);
    CHECK_EQ(snap_of(s).drifting, true);        // 4 s of silence: still drifting
    run(s, 1);
    CHECK_EQ(snap_of(s).drifting, false);       // 5 s: the server stopped

    // A drift at the end of study does not carry into the break, and ticks there are ignored
    focus_for(s, 10, CameraState::UNFOCUSED);
    CHECK_EQ(snap_of(s).drifting, true);
    run(s, 15 * 60);
    CHECK_EQ((int)s.phase(), (int)Phase::BREAK);
    CHECK_EQ(snap_of(s).drifting, false);
    focus_for(s, 30, CameraState::UNFOCUSED);
    CHECK_EQ(snap_of(s).drifting, false);

    SessionCore idle;
    focus_for(idle, 30, CameraState::UNFOCUSED);
    CHECK_EQ(snap_of(idle).drifting, false);
}

static void test_drift_sources()
{
    puts("laptop and camera drift together");
    SessionCore s;
    s.start(25, 5);
    s.setDrift(true, "Idle", DriftKind::IDLE);
    SessionSnapshot snap = snap_of(s);
    CHECK_EQ((int)snap.drift_kind, (int)DriftKind::IDLE);
    CHECK_STR(snap.drift_label, "Idle");

    focus_for(s, 10, CameraState::UNFOCUSED, "Away");
    snap = snap_of(s);
    CHECK_EQ((int)snap.drift_kind, (int)DriftKind::IDLE);   // the laptop's drift is shown
    CHECK_EQ(snap.drift_s, 10);

    s.setDrift(false, nullptr);
    snap = snap_of(s);
    CHECK_EQ(snap.drifting, true);              // the camera still says drifting
    CHECK_EQ((int)snap.drift_kind, (int)DriftKind::CAMERA);
    CHECK_STR(snap.drift_label, "Away");
    CHECK_EQ(snap.drift_s, 10);                 // still the same drift

    focus_for(s, 3, CameraState::FOCUSED);
    snap = snap_of(s);
    CHECK_EQ(snap.drifting, false);
    CHECK_EQ(snap.drift_s, 0);

    // The camera going back to focus does not clear the laptop's drift
    s.setDrift(true, "YouTube");
    focus_for(s, 5, CameraState::FOCUSED);
    snap = snap_of(s);
    CHECK_EQ((int)snap.drift_kind, (int)DriftKind::SITE);
    CHECK_STR(snap.drift_label, "YouTube");
}

static void test_end_break_and_set()
{
    puts("end break, end session");
    SessionEvent e = {};
    {
        SessionCore s;
        CHECK_EQ(s.endBreak(&e), false);        // idle
        CHECK_EQ(s.endSet(&e), false);
        s.start(25, 5);
        CHECK_EQ(s.endBreak(&e), false);        // studying
        run(s, 25 * 60);
        run(s, 60);
        CHECK_EQ(s.endBreak(&e), true);         // break -> session 2 now
        CHECK_EQ((int)e.type, (int)SessionEventType::BREAK_ENDED);
        SessionSnapshot snap = snap_of(s);
        CHECK_EQ((int)snap.phase, (int)Phase::STUDY);
        CHECK_EQ(snap.session_index, 2);
        CHECK_EQ(snap.remaining_s, 25 * 60);
    }
    {
        SessionCore s;
        s.start(15, 3);
        for (int i = 0; i < 4; i++) {
            run(s, 15 * 60);
            if (i < 3) s.endBreak(&e);
        }
        CHECK_EQ((int)s.phase(), (int)Phase::LONG_REST);
        CHECK_EQ(s.endBreak(&e), true);         // long rest -> idle
        CHECK_EQ((int)e.type, (int)SessionEventType::SET_ENDED);
        CHECK_EQ((int)s.phase(), (int)Phase::IDLE);
        CHECK_EQ(snap_of(s).session_index, 1);
        CHECK_EQ(s.xp(), 60);
    }
    {
        SessionCore s;
        s.start(15, 3);
        run(s, 15 * 60);
        const int xp = s.xp();
        CHECK_EQ(s.endSet(&e), true);           // during the break: idle, nothing scored
        CHECK_EQ((int)e.type, (int)SessionEventType::SET_ENDED);
        CHECK_EQ((int)s.phase(), (int)Phase::IDLE);
        CHECK_EQ(s.xp(), xp);
    }
    {
        SessionCore s;
        s.start(25, 5);
        run(s, 5 * 60);
        CHECK_EQ(s.endSet(&e), true);           // while studying: scored, then idle
        CHECK_EQ((int)e.type, (int)SessionEventType::ENDED_EARLY);
        CHECK_EQ(e.result.elapsed_min, 5);
        CHECK_EQ((int)s.phase(), (int)Phase::IDLE);
    }
}

static void test_ready()
{
    puts("ready after a break");
    SessionEvent e = {};
    {
        SessionCore s;
        s.start(25, 5);
        run(s, 30 * 60);
        CHECK_EQ((int)s.phase(), (int)Phase::READY);
        const int xp = s.xp();
        CHECK_EQ(s.endSet(&e), true);           // Finish for now: idle, nothing scored
        CHECK_EQ((int)e.type, (int)SessionEventType::SET_ENDED);
        CHECK_EQ((int)s.phase(), (int)Phase::IDLE);
        CHECK_EQ(snap_of(s).session_index, 1);
        CHECK_EQ(s.xp(), xp);
    }
    {
        SessionCore s;
        s.start(25, 5);
        run(s, 30 * 60);
        CHECK_EQ(s.endEarly(&e), false);        // nothing to end early
        CHECK_EQ(s.start(45, 9), true);         // a start from the laptop continues the set
        SessionSnapshot snap = snap_of(s);
        CHECK_EQ((int)snap.phase, (int)Phase::STUDY);
        CHECK_EQ(snap.session_index, 2);
        CHECK_EQ(snap.remaining_s, 45 * 60);
    }
    {
        SessionCore s;
        s.start(25, 5);
        run(s, 25 * 60 + 60);
        CHECK_EQ(s.endBreak(&e), true);         // Start now, mid-break
        CHECK_EQ((int)e.type, (int)SessionEventType::BREAK_ENDED);
        CHECK_EQ(snap_of(s).session_index, 2);
    }
}

static void test_test_timing()
{
    puts("test timing: 30 s phases, 10 s minutes");
    SessionCore s;
    SessionEvent e = {};
    CHECK_EQ(s.start(25, 5, true), true);
    SessionSnapshot snap = snap_of(s);
    CHECK_EQ(snap.remaining_s, 30);
    CHECK_EQ(snap.test, true);
    CHECK_EQ(snap.study_min, 25);               // the real lengths are kept

    // 3 s of drift in the second "minute" makes it a drift minute
    run(s, 10);
    s.setDrift(true, "YouTube");
    run(s, 3);
    s.setDrift(false, nullptr);
    CHECK_EQ(run(s, 17, &e), (int)SessionEventType::STUDY_ENDED);
    CHECK_EQ(e.result.elapsed_min, 3);
    CHECK_EQ(e.result.focused_min, 2);
    CHECK_EQ(e.result.test, true);
    CHECK_EQ(snap_of(s).remaining_s, 30);       // break

    CHECK_EQ(run(s, 30), (int)SessionEventType::BREAK_OVER);
    CHECK_EQ(s.endBreak(&e), true);
    for (int i = 2; i <= 4; i++) {
        CHECK_EQ(run(s, 30), (int)SessionEventType::STUDY_ENDED);
        if (i < 4) {
            CHECK_EQ(run(s, 30), (int)SessionEventType::BREAK_OVER);
            s.endBreak(&e);
        }
    }
    CHECK_EQ((int)s.phase(), (int)Phase::LONG_REST);
    CHECK_EQ(snap_of(s).remaining_s, 30);
    CHECK_EQ(run(s, 30), (int)SessionEventType::SET_ENDED);

    // The next normal start is normal again
    s.start(25, 5);
    snap = snap_of(s);
    CHECK_EQ(snap.remaining_s, 25 * 60);
    CHECK_EQ(snap.test, false);
}

static void test_session_log()
{
    puts("session log");
    SessionLog log = {};
    SessionResult r = {};
    r.focused_min = 20;
    r.session_index = 1;
    logSession(&log, r, true, 100);
    CHECK_EQ(log.sessions, 1);
    CHECK_EQ(log.sets, 0);
    CHECK_EQ(log.focused_min, 20);
    CHECK_EQ(logToday(log, 100), 1);
    CHECK_EQ(logToday(log, 101), 0);            // a new day starts at 0
    CHECK_EQ(logToday(log, 0), 0);              // clock not set

    r.focused_min = 4;                          // ended early: minutes count, the session doesn't
    logSession(&log, r, false, 100);
    CHECK_EQ(log.sessions, 1);
    CHECK_EQ(log.focused_min, 24);
    CHECK_EQ(logToday(log, 100), 1);

    r.focused_min = 25;
    r.session_index = 4;                        // the 4th session finishes the set
    logSession(&log, r, true, 101);
    CHECK_EQ(log.sessions, 2);
    CHECK_EQ(log.sets, 1);
    CHECK_EQ(logToday(log, 101), 1);            // the next day counts from 1
    CHECK_EQ(logToday(log, 100), 0);

    logSession(&log, r, true, 0);               // clock not set: totals only
    CHECK_EQ(log.sessions, 3);
    CHECK_EQ(log.sets, 2);
    CHECK_EQ(logToday(log, 101), 1);

    // Through the rules: a full set of four gives a session_index of 1..4
    SessionCore s;
    SessionLog full = {};
    SessionEvent e = {};
    s.start(15, 3);
    for (int i = 0; i < 4; i++) {
        CHECK_EQ(run(s, 15 * 60, &e), (int)SessionEventType::STUDY_ENDED);
        CHECK_EQ(e.result.session_index, i + 1);
        logSession(&full, e.result, true, 7);
        if (i < 3) s.endBreak(&e);
    }
    CHECK_EQ(full.sessions, 4);
    CHECK_EQ(full.sets, 1);
    CHECK_EQ(full.focused_min, 60);
    CHECK_EQ(logToday(full, 7), 4);

    // A set under test timing counts its sessions and set, but no minutes
    SessionCore t;
    SessionLog test_log = {};
    t.start(25, 5, true);
    for (int i = 0; i < 4; i++) {
        CHECK_EQ(run(t, 30, &e), (int)SessionEventType::STUDY_ENDED);
        logSession(&test_log, e.result, true, 7);
        if (i < 3) {
            run(t, 30);
            t.endBreak(&e);
        }
    }
    CHECK_EQ(test_log.sessions, 4);
    CHECK_EQ(test_log.sets, 1);
    CHECK_EQ(test_log.focused_min, 0);
    CHECK_EQ(logToday(test_log, 7), 4);
}

static void test_copy_utf8()
{
    puts("UTF-8 safe copy");
    char buf[6];
    copy_utf8(buf, sizeof(buf), "Mochi");
    CHECK_STR(buf, "Mochi");
    copy_utf8(buf, sizeof(buf), "Mochiko");
    CHECK_STR(buf, "Mochi");
    copy_utf8(buf, sizeof(buf), "Zo\xC3\xAB!");                 // fits exactly: 5 bytes
    CHECK_STR(buf, "Zo\xC3\xAB!");
    copy_utf8(buf, sizeof(buf), "Zoey\xC3\xAB");                // the 2-byte letter would be cut
    CHECK_STR(buf, "Zoey");
    copy_utf8(buf, sizeof(buf), "\xE2\x9C\x93\xE2\x9C\x93");    // two 3-byte checkmarks
    CHECK_STR(buf, "\xE2\x9C\x93");
    copy_utf8(buf, sizeof(buf), nullptr);
    CHECK_STR(buf, "");
}

int main()
{
    test_break_ranges();
    test_study_lengths();
    test_long_rest_for_long_sessions();
    test_earned_in_snapshot();
    test_full_session_and_break();
    test_drift_minutes();
    test_multiplier();
    test_penalty_floor();
    test_set_and_long_rest();
    test_end_early();
    test_camera_drift();
    test_camera_timeout_and_phases();
    test_drift_sources();
    test_end_break_and_set();
    test_ready();
    test_test_timing();
    test_session_log();
    test_copy_utf8();
    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    puts("all passed");
    return 0;
}
