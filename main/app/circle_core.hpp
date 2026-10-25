#pragma once
// Study circle (linked play) rules. Plain C++ with no ESP-IDF dependencies, so it can be
// tested on a PC. comms/circle.cpp feeds it packets from ESP-NOW, this device's session and
// a clock in ms, and carries out what it asks for.
//
// Every Pomo nearby shows up on its own: each one broadcasts a beacon about every 1.5 s, and
// one that has gone quiet for 6 s leaves. Studying together is by invite:
//   1. A host invites up to two free friends. The invite is resent every second until it's
//      answered, for up to 30 s.
//   2. Accepting joins the host's group: the guest's beacon carries the group id from then
//      on. Declining sends a decline.
//   3. The host's session starts once every invite is answered and someone joined, or when
//      the host presses Start. Guests waiting in the group start when the host's beacon
//      shows it studying, with the invite's lengths.
//   4. The group stays in step for the whole set: a member on a break, or waiting at READY,
//      starts the next session when another member's beacon shows them studying it.
//   5. Ending the set (or the session early) leaves the group.
// The XP multiplier counts group members who are studying; one of them drifting pauses it.
#include <stddef.h>
#include <stdint.h>

#include "session_core.hpp"

// What a device tells the circle about itself
struct Beacon {
    char name[11];      // the student's name, or the pet's when none is set (up to 10 bytes)
    Phase phase;
    uint8_t index;      // session of the set, 1-4 (during a break: the one just finished)
    bool focused;       // not drifting
    uint8_t pct;        // last scored session's focus %, 0-100
    uint16_t seq;       // counts up with every beacon sent, to spot missed ones
    uint32_t group;     // the study group it hosts or joined, 0 for none

    bool studying() const { return phase == Phase::STUDY; }
    bool inSet() const { return phase != Phase::IDLE; }
};

// "Come study with me", from a host to one friend
struct Invite {
    uint8_t to[6];
    uint32_t group;
    uint8_t study, brk;     // the host's lengths in minutes
    bool test;              // test timing
};

// "No thanks" (or "busy"), back to the host
struct Decline {
    uint8_t to[6];
    uint32_t group;
};

enum class MsgType : uint8_t { BEACON, INVITE, DECLINE };

// On the air: "POMO", version, type, then the message. Little endian.
//   beacon:  phase, index, flags, pct, seq (2), group (4), name (11, NUL padded)
//   invite:  to (6), group (4), study, brk, flags
//   decline: to (6), group (4)
namespace circle_wire {
    static constexpr uint8_t VERSION = 2;
    static constexpr size_t HEADER = 6;
    static constexpr size_t BEACON_SIZE = HEADER + 21;
    static constexpr size_t INVITE_SIZE = HEADER + 13;
    static constexpr size_t DECLINE_SIZE = HEADER + 10;
    static constexpr size_t MAX_SIZE = BEACON_SIZE;
    static constexpr uint8_t FLAG_FOCUSED = 0x01;   // beacon
    static constexpr uint8_t FLAG_TEST = 0x01;      // invite

    // Each writes its message into `out` (at least MAX_SIZE long) and returns its length
    size_t encode(const Beacon &b, uint8_t *out);
    size_t encode(const Invite &m, uint8_t *out);
    size_t encode(const Decline &m, uint8_t *out);

    // False for anything that isn't a Pomo message of this version
    bool type(const uint8_t *data, size_t len, MsgType *out);
    bool decode(const uint8_t *data, size_t len, Beacon *out);
    bool decode(const uint8_t *data, size_t len, Invite *out);
    bool decode(const uint8_t *data, size_t len, Decline *out);
}

struct CirclePeer {
    uint8_t mac[6];
    Beacon beacon;
    uint32_t last_heard_ms;
};

// What a friend nearby is to this device, for the Circle screen
enum class FriendStatus : uint8_t {
    FREE,       // can be invited
    BUSY,       // in a set or a group of their own
    INVITED,    // this device invited them and is waiting for an answer
    DECLINED,   // said no to this device's invite
    MEMBER,     // in this device's group: waiting to start, or studying together
};

// This device's part in a study group
enum class GroupRole : uint8_t {
    NONE,
    HOSTING,    // invited friends; the session hasn't started yet
    INVITED,    // a friend's invite is waiting for an answer here
    JOINED,     // accepted; waiting for the host to start
    IN_SET,     // in a set with the group
};

// Something the session should do, from takeAction()
struct GroupAction {
    enum Type : uint8_t {
        NONE,
        START,  // start session 1 with these lengths
        NEXT,   // the group moved on: end the break (or READY) and start the next session
    } type;
    uint8_t study, brk;
    bool test;
};

class CircleCore {
public:
    static constexpr int MAX_PEERS = 4;
    static constexpr int MAX_GROUP = 3;     // the host and two guests: 3x, the cap
    static constexpr uint32_t PEER_TIMEOUT_MS = 6000;
    static constexpr uint32_t BEACON_INTERVAL_MS = 1500;
    static constexpr uint32_t INVITE_TIMEOUT_MS = 30000;
    static constexpr uint32_t INVITE_RESEND_MS = 1000;
    static constexpr uint32_t DECLINED_SHOWN_MS = 10000;    // "Declined" stays on screen
    static constexpr int DECLINE_SENDS = 3;                 // a decline goes out this often

    // This device's MAC, so it knows which invites and declines are meant for it
    void setMac(const uint8_t mac[6]);

    // ---- In from the radio ----

    // A beacon from `mac`. Returns true when what the circle shows changed: a friend joined,
    // or one's name, phase, focus, % or group changed. A fifth device is ignored until one
    // leaves. Also moves the group along (someone joined or left, the host started, a member
    // started the next session).
    bool onBeacon(const uint8_t mac[6], const Beacon &b, uint32_t now_ms);
    void onInvite(const uint8_t from[6], const Invite &m, uint32_t now_ms);
    void onDecline(const uint8_t from[6], const Decline &m, uint32_t now_ms);

    // ---- This device ----

    // The session's phase and index, whenever they may have changed
    void setSelf(Phase phase, int index);

    // From the UI. Each returns false if it doesn't apply right now.
    // Invite a free friend; the first invite opens a group with id `new_group` (not 0)
    // and these lengths, and later ones use the same lengths.
    bool invite(const uint8_t mac[6], int study, int brk, bool test, uint32_t new_group, uint32_t now_ms);
    bool startHosted();     // host: start now, with whoever has joined
    bool cancelHosted();    // host: close the group before it starts
    bool accept();          // guest: join the group of the invite on screen
    bool decline();         // guest: say no to the invite, or leave a group not started yet

    // ---- Time ----

    // Drops friends not heard from for PEER_TIMEOUT_MS. Returns true if anyone left.
    bool expire(uint32_t now_ms);
    // Call every few hundred ms: expire(), invites running out, a host whose every invite
    // is answered starting the session
    void tick(uint32_t now_ms);

    // ---- Out ----

    // The next message due to go out (invites resent, declines), or false when none is.
    // Call until it returns false.
    bool nextMessage(uint32_t now_ms, uint8_t *out, size_t *len);
    // What the session should do, once (NONE when nothing)
    GroupAction takeAction();
    // True once after anything the UI shows changed (friends, statuses, role)
    bool takeChanged();

    // ---- For the session, the beacon and the UI ----

    int count() const { return count_; }
    const CirclePeer &peer(int i) const { return peers_[i]; }
    FriendStatus status(int i) const;

    // For SessionCore::setCircle: group members who are studying, and whether one is drifting
    int membersStudying() const;
    bool memberDrifting() const;

    GroupRole role() const { return role_; }
    // The group this device's beacon carries: hosted or joined, 0 for none
    uint32_t group() const { return group_; }
    // HOSTING: room for another invite. NONE: free to start a group.
    bool canInvite() const;
    // HOSTING with someone joined
    bool canStart() const;
    // INVITED / JOINED: the host's name ("" if not heard) and the invite's lengths.
    // HOSTING: this group's lengths.
    const char *hostName() const;
    const Invite &invitation() const { return invitation_; }

private:
    struct Invitee {
        uint8_t mac[6];
        FriendStatus status;    // INVITED, DECLINED or MEMBER (joined)
        uint32_t until_ms;      // INVITED: when it runs out; DECLINED: when it stops showing
        uint32_t next_send_ms;
    };
    struct PendingDecline {
        Decline msg;
        int sends_left;
        uint32_t next_send_ms;
    };
    static constexpr int MAX_INVITEES = MAX_GROUP - 1;
    static constexpr int MAX_DECLINES = 3;

    int find(const uint8_t mac[6]) const;
    Invitee *invitee(const uint8_t mac[6]);
    bool isMember(const Beacon &b) const { return group_ != 0 && b.group == group_; }
    void queueDecline(const uint8_t to[6], uint32_t group);
    void leaveGroup();
    void setAction(GroupAction::Type type, const Invite &lengths);

    CirclePeer peers_[MAX_PEERS] = {};
    int count_ = 0;
    uint8_t mac_[6] = {};

    Phase phase_ = Phase::IDLE;
    int index_ = 1;

    GroupRole role_ = GroupRole::NONE;
    uint32_t group_ = 0;
    // HOSTING and IN_SET as host: the group's lengths. INVITED / JOINED / IN_SET as guest:
    // the invite (its `to` is this device), and the host's MAC in host_.
    Invite invitation_ = {};
    uint8_t host_[6] = {};
    uint32_t invite_until_ms_ = 0;
    Invitee invitees_[MAX_INVITEES] = {};
    int invitee_count_ = 0;
    PendingDecline declines_[MAX_DECLINES] = {};
    int decline_count_ = 0;

    GroupAction action_ = {};
    bool started_ = false;      // START asked for, waiting for the session to show STUDY
    int next_for_ = 0;          // NEXT asked for this session index
    bool changed_ = false;
};
