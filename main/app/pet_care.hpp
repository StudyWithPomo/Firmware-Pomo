#pragma once
// Pet care: gears earned by studying, spent on feeding, and hunger over time. Plain C++ with no
// ESP-IDF or LVGL, so it can be tested on a PC (test/pet_care_test.cpp). AppState owns a
// PetCare and calls these from the LVGL task. The full spec, for the dashboard too, is
// docs/PET_PLAY_AND_CARE.md (Part 2).
//
// Two clocks:
//   now     Unix seconds from the laptop, or 0 while the time isn't known. Hunger (hours)
//           runs on this, so a power cycle can't starve the pet.
//   uptime  seconds since boot, always known. The short windows (full for 30 min after a
//           feed, the stuffed limit) run on this, so they work before the time is set.
#include <stdint.h>

#include <app/session_core.hpp>

enum class Hunger : uint8_t { FULL, CONTENT, PECKISH, HUNGRY };
enum class FeedResult : uint8_t { FED, EMPTY, STUFFED };

struct PetCare {
    // Saved
    int gears = 0;
    long fed_at = 0;            // Unix seconds of the last feed; 0 = never
    // This boot only
    long fed_uptime = -1;       // uptime of the last feed, -1 = none yet
    int feeds_n = 0;            // feeds in the current stuffed window
    long feeds_from = 0;        // uptime the window started
};

namespace pet_care {

constexpr int MAX_GEARS = 12;
constexpr int GEARS_PER_SESSION = 1;     // every study session whose timer ran out
constexpr int GEARS_PER_SET = 2;         // more when that session finishes the set
constexpr long FULL_FOR_S = 30 * 60;
constexpr long PECKISH_AFTER_S = 12 * 3600;
constexpr long HUNGRY_AFTER_S = 24 * 3600;
constexpr int STUFFED_AFTER = 3;         // feeds per window; the next is refused
constexpr long STUFFED_WINDOW_S = 30 * 60;

// A scored study session. Returns the gears actually added (0 when ended early or full).
int earn(PetCare *care, const SessionResult &result, bool completed);

// Spend a gear on a feed, unless there are none or the pet is stuffed
FeedResult feed(PetCare *care, long now, long uptime);

Hunger hunger(const PetCare &care, long now, long uptime);

// Call whenever the time is known (once a second is fine). A pet never fed starts its clock
// now (so a new pet is content, not hungry), and so does one fed "in the future" (the clock
// went backwards).
void clock_known(PetCare *care, long now);

// "full", "content", "peckish", "hungry" (the protocol's names)
const char *hunger_name(Hunger hunger);

// The pet message's event for a feed attempt: "fed", "empty", "stuffed". The same whether the
// feed came from the knob, the Playground's button or the laptop's {"t":"feed"}.
const char *feed_event(FeedResult result);

} // namespace pet_care
