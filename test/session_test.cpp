// Host test for the study session rules (main/app/session_core.cpp). No ESP-IDF needed:
//   g++ -std=c++17 -Wall -I main test/session_test.cpp main/app/session_core.cpp -o session_test && ./session_test

#include <stdio.h>
#include <string.h>

#include "app/session_core.hpp"

static int failures = 0;

#define CHECK_EQ(actual, expected)                                                        \
    do {                                                                                  \
        const long a_ = (long)(actual), e_ = (long)(expected);                            \
        if (a_ != e_) {                                                                   \
            printf("  FAIL %s:%d  %s == %ld, expected %ld\n", __FILE__, __LINE__, #actual, a_, e_); \
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
    CHECK_EQ(SessionCore::snapStudy(30), 25);
    CHECK_EQ(SessionCore::snapStudy(100), 60);
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

    CHECK_EQ(run(s, 5 * 60), (int)SessionEventType::BREAK_ENDED);
    s.snapshot(&snap);
    CHECK_EQ((int)snap.phase, (int)Phase::STUDY);
    CHECK_EQ(snap.session_index, 2);
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
        CHECK_EQ(run(s, 3 * 60), (int)SessionEventType::BREAK_ENDED);
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

int main()
{
    test_break_ranges();
    test_full_session_and_break();
    test_drift_minutes();
    test_multiplier();
    test_penalty_floor();
    test_set_and_long_rest();
    test_end_early();
    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    puts("all passed");
    return 0;
}
