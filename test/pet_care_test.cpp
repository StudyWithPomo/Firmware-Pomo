// Host test for the pet care rules (main/app/pet_care.cpp), the cases in
// docs/PET_PLAY_AND_CARE.md Part 2. No ESP-IDF needed:
//   g++ -std=c++17 -Wall -I main test/pet_care_test.cpp main/app/pet_care.cpp main/app/session_core.cpp main/utilities.cpp -o pet_care_test && ./pet_care_test

#include <stdio.h>
#include <string.h>

#include "app/pet_care.hpp"

static int failures = 0;

#define CHECK_EQ(actual, expected)                                                        \
    do {                                                                                  \
        const long a_ = (long)(actual), e_ = (long)(expected);                            \
        if (a_ != e_) {                                                                   \
            printf("  FAIL %s:%d  %s == %ld, expected %ld\n", __FILE__, __LINE__, #actual, a_, e_); \
            failures++;                                                                   \
        }                                                                                 \
    } while (0)

using namespace pet_care;

static const long T = 1792790040;   // Fri 23 Oct 2026, a time the laptop might send
static const long MIN = 60, HOUR = 3600, DAY = 24 * HOUR;

static SessionResult session(int index)
{
    SessionResult r = {};
    r.session_index = index;
    return r;
}

static void test_earning()
{
    puts("earning gears");
    PetCare c;
    CHECK_EQ(earn(&c, session(1), true), 1);
    CHECK_EQ(c.gears, 1);
    CHECK_EQ(earn(&c, session(2), false), 0);       // ended early: nothing
    CHECK_EQ(earn(&c, session(4), true), 3);        // the 4th finishes the set: 1 + 2
    CHECK_EQ(c.gears, 4);
    SessionResult poor = session(2);
    poor.penalty = true;
    CHECK_EQ(earn(&c, poor, true), 1);              // any focus score counts once completed

    PetCare nearly;
    nearly.gears = 11;
    CHECK_EQ(earn(&nearly, session(4), true), 1);   // capped at 12, reports what was added
    CHECK_EQ(nearly.gears, MAX_GEARS);
    CHECK_EQ(earn(&nearly, session(1), true), 0);
    CHECK_EQ(nearly.gears, MAX_GEARS);
}

static void test_feeding()
{
    puts("feeding");
    PetCare c;
    c.fed_at = T - DAY;
    CHECK_EQ((int)feed(&c, T, 100), (int)FeedResult::EMPTY);
    CHECK_EQ(c.gears, 0);
    CHECK_EQ(c.fed_at, T - DAY);                    // unchanged

    c.gears = 5;
    CHECK_EQ((int)feed(&c, T, 100), (int)FeedResult::FED);
    CHECK_EQ((int)feed(&c, T + 5 * MIN, 100 + 5 * MIN), (int)FeedResult::FED);
    CHECK_EQ((int)feed(&c, T + 10 * MIN, 100 + 10 * MIN), (int)FeedResult::FED);
    CHECK_EQ(c.gears, 2);
    CHECK_EQ(c.fed_at, T + 10 * MIN);
    CHECK_EQ((int)feed(&c, T + 11 * MIN, 100 + 11 * MIN), (int)FeedResult::STUFFED);
    CHECK_EQ(c.gears, 2);                           // a refused feed keeps its gear
    // The window started with the first feed; 31 minutes after it, feeding works again
    CHECK_EQ((int)feed(&c, T + 31 * MIN, 100 + 31 * MIN), (int)FeedResult::FED);
    CHECK_EQ(c.gears, 1);
}

static void test_hunger()
{
    puts("hunger over time");
    PetCare c;
    c.gears = 1;
    feed(&c, T, 1000);
    // This boot: full on the uptime clock too
    CHECK_EQ((int)hunger(c, T + 29 * MIN, 1000 + 29 * MIN), (int)Hunger::FULL);
    CHECK_EQ((int)hunger(c, T + 31 * MIN, 1000 + 31 * MIN), (int)Hunger::CONTENT);
    // After a restart (no feed this boot): from fed_at alone
    PetCare later;
    later.fed_at = T;
    CHECK_EQ((int)hunger(later, T + 29 * MIN, 5), (int)Hunger::FULL);
    CHECK_EQ((int)hunger(later, T + 31 * MIN, 5), (int)Hunger::CONTENT);
    CHECK_EQ((int)hunger(later, T + 12 * HOUR - 1, 5), (int)Hunger::CONTENT);
    CHECK_EQ((int)hunger(later, T + 12 * HOUR, 5), (int)Hunger::PECKISH);
    CHECK_EQ((int)hunger(later, T + 24 * HOUR, 5), (int)Hunger::HUNGRY);
    CHECK_EQ((int)hunger(later, T + 10 * DAY, 5), (int)Hunger::HUNGRY);  // and stops there
}

static void test_clock()
{
    puts("clock rules");
    // Unknown time: content, and it holds still
    PetCare old;
    old.fed_at = T - 3 * DAY;
    CHECK_EQ((int)hunger(old, 0, 50), (int)Hunger::CONTENT);
    // Fed while the time is unknown: full for 30 min of uptime, fed_at not saved
    PetCare blind;
    blind.gears = 2;
    CHECK_EQ((int)feed(&blind, 0, 50), (int)FeedResult::FED);
    CHECK_EQ(blind.fed_at, 0);
    CHECK_EQ((int)hunger(blind, 0, 50 + 10 * MIN), (int)Hunger::FULL);
    CHECK_EQ((int)hunger(blind, 0, 50 + 40 * MIN), (int)Hunger::CONTENT);

    // A brand-new pet: its clock starts when the time is known, so it's content
    PetCare fresh;
    clock_known(&fresh, T);
    CHECK_EQ(fresh.fed_at, T);
    CHECK_EQ((int)hunger(fresh, T + HOUR, 9000), (int)Hunger::CONTENT);
    clock_known(&fresh, T + 2 * DAY);               // already set: left alone
    CHECK_EQ(fresh.fed_at, T);
    CHECK_EQ((int)hunger(fresh, T + 2 * DAY, 9000), (int)Hunger::HUNGRY);

    // The clock went backwards, before fed_at: it starts over from now
    clock_known(&fresh, T - DAY);
    CHECK_EQ(fresh.fed_at, T - DAY);
    CHECK_EQ((int)hunger(fresh, T - DAY + HOUR, 9000), (int)Hunger::CONTENT);
    clock_known(&fresh, 0);                         // unknown: ignored
    CHECK_EQ(fresh.fed_at, T - DAY);
}

// The laptop's {"t":"feed"} and the device's Feed call the same feed(): one gear store, one
// stuffed window, and the same event names in the pet message
static void test_feed_from_anywhere()
{
    puts("feeds from the laptop and the device share the rules");
    PetCare c;
    c.gears = 4;
    const long up = 500;
    // device, laptop, device: three feeds in a few minutes, whoever sent them
    CHECK_EQ((int)feed(&c, T, up), (int)FeedResult::FED);
    CHECK_EQ((int)feed(&c, T + 60, up + 60), (int)FeedResult::FED);
    CHECK_EQ((int)feed(&c, T + 120, up + 120), (int)FeedResult::FED);
    CHECK_EQ(c.gears, 1);
    // a fourth from the laptop is refused like one from the knob, and keeps its gear
    CHECK_EQ((int)feed(&c, T + 180, up + 180), (int)FeedResult::STUFFED);
    CHECK_EQ(c.gears, 1);
    if (strcmp(feed_event(FeedResult::FED), "fed") != 0 || strcmp(feed_event(FeedResult::EMPTY), "empty") != 0 ||
        strcmp(feed_event(FeedResult::STUFFED), "stuffed") != 0) {
        puts("  FAIL feed event names");
        failures++;
    }
    // and with no gears at all it's empty, from anywhere
    PetCare none;
    CHECK_EQ((int)feed(&none, T, up), (int)FeedResult::EMPTY);
    CHECK_EQ(none.gears, 0);
}

static void test_names()
{
    puts("names");
    if (hunger_name(Hunger::FULL)[0] != 'f' || hunger_name(Hunger::CONTENT)[0] != 'c' ||
        hunger_name(Hunger::PECKISH)[0] != 'p' || hunger_name(Hunger::HUNGRY)[0] != 'h') {
        puts("  FAIL hunger names");
        failures++;
    }
}

int main()
{
    test_earning();
    test_feeding();
    test_hunger();
    test_clock();
    test_feed_from_anywhere();
    test_names();
    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    puts("all passed");
    return 0;
}
