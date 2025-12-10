#include "system.hpp"
#include <ctime>
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
    return {true, -60};
}

SystemAPI::BluetoothStatus SystemAPI::getBluetoothStatus() const {
    // Placeholder implementation
    return {true};
}

std::string SystemAPI::getCurrentTime() const {
    std::time_t now = std::time(nullptr);
    char buf[10];
    std::strftime(buf, sizeof(buf), "%I:%M %p", std::localtime(&now));
    return std::string(buf);
}

std::string SystemAPI::getCurrentDate() const {
    std::time_t now = std::time(nullptr);
    char buf[30];
    std::strftime(buf, sizeof(buf), "%A, %B %d", std::localtime(&now));
    return std::string(buf);
}