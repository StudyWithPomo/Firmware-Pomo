#include "system.hpp"
#include <ctime>

SystemAPI &SystemAPI::instance() {
    static SystemAPI instance;
    return instance;
}

int SystemAPI::setCurrentTime(long epochSeconds) const {
    timespec ts;
    ts.tv_sec = epochSeconds;
    ts.tv_nsec = 0;
    return clock_settime(CLOCK_REALTIME, &ts);
}

long SystemAPI::getCurrentTime() const {
    timespec ts;
    return clock_gettime(CLOCK_REALTIME, &ts) == 0 ? ts.tv_sec : -1;
}
