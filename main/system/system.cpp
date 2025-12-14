#include "system.hpp"
#include "time.h"
#include <ctime>
#include <cstring>
#include <string>
#include <stdexcept>

SystemAPI &SystemAPI::instance() {
    static SystemAPI instance;
    return instance;
}

SystemAPI::BatteryStatus SystemAPI::getBatteryStatus() const {
    // Placeholder implementation
    return {true, 85};
}

SystemAPI::WiFiStatus SystemAPI::getWiFiStatus() const {
    // Placeholder implementation
    return {true, "Fios-S2f3Y", -60};
}

SystemAPI::BluetoothStatus SystemAPI::getBluetoothStatus() const {
    // Placeholder implementation
    return {true};
}

SystemAPI::CellularStatus SystemAPI::getCellularStatus() const {
    // Placeholder implementation
    CellularStatus status{
        true,
        "Cube",
        -30
    };
    return status;
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

bool operator!=(const SystemAPI::CellularStatus& a, const SystemAPI::CellularStatus& b) {
    return (a.simCardPresent != b.simCardPresent) ||
           strcmp(a.carrierName, b.carrierName) ||
           (a.signalStrength != b.signalStrength);
}