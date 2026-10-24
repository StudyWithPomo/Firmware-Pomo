#pragma once

class SystemAPI {
public:
    static SystemAPI &instance();

    // Get current time in seconds since epoch
    long getCurrentTime() const;

    // Set current time in seconds since epoch
    // Returns 0 on success, non-zero on failure
    int setCurrentTime(long epochSeconds) const;
};
