#include "pet_care.hpp"

namespace pet_care {

int earn(PetCare *care, const SessionResult &result, bool completed)
{
    if (!completed) return 0;
    int want = GEARS_PER_SESSION;
    if (result.session_index >= SessionCore::SESSIONS_PER_SET) want += GEARS_PER_SET;
    const int room = MAX_GEARS - care->gears;
    const int added = want < room ? want : (room > 0 ? room : 0);
    care->gears += added;
    return added;
}

FeedResult feed(PetCare *care, long now, long uptime)
{
    if (care->gears <= 0) return FeedResult::EMPTY;
    if (care->feeds_n > 0 && uptime - care->feeds_from >= STUFFED_WINDOW_S) care->feeds_n = 0;
    if (care->feeds_n >= STUFFED_AFTER) return FeedResult::STUFFED;

    care->gears--;
    if (care->feeds_n++ == 0) care->feeds_from = uptime;
    care->fed_uptime = uptime;
    if (now > 0) care->fed_at = now;
    return FeedResult::FED;
}

Hunger hunger(const PetCare &care, long now, long uptime)
{
    if (care.fed_uptime >= 0 && uptime - care.fed_uptime < FULL_FOR_S) return Hunger::FULL;
    if (now <= 0 || care.fed_at <= 0) return Hunger::CONTENT; // time unknown: hunger holds still
    const long since = now - care.fed_at;
    if (since < FULL_FOR_S) return Hunger::FULL;
    if (since < PECKISH_AFTER_S) return Hunger::CONTENT;
    if (since < HUNGRY_AFTER_S) return Hunger::PECKISH;
    return Hunger::HUNGRY;
}

void clock_known(PetCare *care, long now)
{
    if (now <= 0) return;
    if (care->fed_at <= 0 || care->fed_at > now) care->fed_at = now;
}

const char *hunger_name(Hunger hunger)
{
    switch (hunger) {
        case Hunger::FULL: return "full";
        case Hunger::PECKISH: return "peckish";
        case Hunger::HUNGRY: return "hungry";
        default: return "content";
    }
}

const char *feed_event(FeedResult result)
{
    switch (result) {
        case FeedResult::EMPTY: return "empty";
        case FeedResult::STUFFED: return "stuffed";
        default: return "fed";
    }
}

} // namespace pet_care
