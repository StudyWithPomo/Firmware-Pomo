#pragma once
// Study circle (linked play) rules. Plain C++ with no ESP-IDF dependencies, so it can be
// tested on a PC. comms/circle.cpp feeds it beacons from ESP-NOW and a clock in ms.
//
// Every Pomo device nearby joins automatically: each one broadcasts a beacon about every
// 1.5 s, and a device that has gone quiet for 6 s leaves the circle. Every friend nearby adds
// to the XP multiplier, studying or not, and one who is studying and drifts pauses the bonus
// (SessionCore::setCircle).
#include <stddef.h>
#include <stdint.h>

// What a device tells the circle about itself
struct Beacon {
    char name[11];      // the student's name, or the pet's when none is set (up to 10 bytes)
    bool in_session;    // studying right now (not on a break)
    bool focused;       // not drifting
    uint8_t pct;        // last scored session's focus %, 0-100
    uint16_t seq;       // counts up with every beacon sent, to spot missed ones
};

// The beacon on the air: "POMO", version, flags, pct, seq (little endian), name (NUL padded)
namespace beacon_wire {
    static constexpr uint8_t VERSION = 1;
    static constexpr size_t SIZE = 4 + 1 + 1 + 1 + 2 + 11;
    static constexpr uint8_t FLAG_IN_SESSION = 0x01;
    static constexpr uint8_t FLAG_FOCUSED = 0x02;

    // Writes SIZE bytes into `out` (at least SIZE long)
    void encode(const Beacon &b, uint8_t *out);
    // False for anything that isn't a Pomo beacon of this version
    bool decode(const uint8_t *data, size_t len, Beacon *out);
}

struct CirclePeer {
    uint8_t mac[6];
    Beacon beacon;
    uint32_t last_heard_ms;
};

class CircleCore {
public:
    static constexpr int MAX_PEERS = 4;
    static constexpr uint32_t PEER_TIMEOUT_MS = 6000;
    static constexpr uint32_t BEACON_INTERVAL_MS = 1500;

    // A beacon from `mac`. Returns true when what the circle shows changed: a friend joined,
    // or one's name, studying, focus or % changed. A fifth device is ignored until one leaves.
    bool onBeacon(const uint8_t mac[6], const Beacon &b, uint32_t now_ms);
    // Drops friends not heard from for PEER_TIMEOUT_MS. Returns true if anyone left.
    bool expire(uint32_t now_ms);

    int count() const { return count_; }
    const CirclePeer &peer(int i) const { return peers_[i]; }

    // For SessionCore::setCircle, with count() (every friend nearby sets the multiplier)
    int friendsInSession() const;
    bool friendDrifting() const;  // a friend who is studying is drifting

private:
    CirclePeer peers_[MAX_PEERS] = {};
    int count_ = 0;
};
