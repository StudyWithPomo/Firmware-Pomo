#pragma once

#include <string>

class SystemAPI {
public:
    static SystemAPI &instance();

    // Battery status and percent
    struct BatteryStatus {
        bool isCharging;
        int percent; // 0 to 100
    };

    // WiFi status and signal
    struct WiFiStatus {
        bool isConnected;
        int signalStrength; // in dBm
    };

    // Bluetooth status
    struct BluetoothStatus {
        bool isEnabled;
    };

    // Get battery status
    BatteryStatus getBatteryStatus() const;

    // Get WiFi status
    WiFiStatus getWiFiStatus() const;

    // Get Bluetooth status
    BluetoothStatus getBluetoothStatus() const;

    // Get current time as string
    std::string getCurrentTime() const;

    // Get current date as string
    std::string getCurrentDate() const;
};