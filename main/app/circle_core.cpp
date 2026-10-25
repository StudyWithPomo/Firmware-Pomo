#include "circle_core.hpp"

#include <string.h>

#include "utilities.h"

static const uint8_t MAGIC[4] = {'P', 'O', 'M', 'O'};

// Unsigned difference, so deadlines still work when the ms clock wraps
static bool reached(uint32_t now_ms, uint32_t deadline_ms) { return (int32_t)(now_ms - deadline_ms) >= 0; }

static void put32(uint8_t *p, uint32_t v)
{
    for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i));
}

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void header(uint8_t *out, size_t size, MsgType type)
{
    memset(out, 0, size);
    memcpy(out, MAGIC, 4);
    out[4] = circle_wire::VERSION;
    out[5] = (uint8_t)type;
}

size_t circle_wire::encode(const Beacon &b, uint8_t *out)
{
    header(out, BEACON_SIZE, MsgType::BEACON);
    out[6] = (uint8_t)b.phase;
    out[7] = b.index;
    out[8] = b.focused ? FLAG_FOCUSED : 0;
    out[9] = b.pct > 100 ? 100 : b.pct;
    out[10] = (uint8_t)(b.seq & 0xFF);
    out[11] = (uint8_t)(b.seq >> 8);
    put32(out + 12, b.group);
    copy_utf8((char *)out + 16, 11, b.name);
    return BEACON_SIZE;
}

size_t circle_wire::encode(const Invite &m, uint8_t *out)
{
    header(out, INVITE_SIZE, MsgType::INVITE);
    memcpy(out + 6, m.to, 6);
    put32(out + 12, m.group);
    out[16] = m.study;
    out[17] = m.brk;
    out[18] = m.test ? FLAG_TEST : 0;
    return INVITE_SIZE;
}

size_t circle_wire::encode(const Decline &m, uint8_t *out)
{
    header(out, DECLINE_SIZE, MsgType::DECLINE);
    memcpy(out + 6, m.to, 6);
    put32(out + 12, m.group);
    return DECLINE_SIZE;
}

bool circle_wire::type(const uint8_t *data, size_t len, MsgType *out)
{
    if (!data || len < HEADER || memcmp(data, MAGIC, 4) != 0 || data[4] != VERSION) return false;
    static const size_t SIZES[] = {BEACON_SIZE, INVITE_SIZE, DECLINE_SIZE};
    if (data[5] > (uint8_t)MsgType::DECLINE || len < SIZES[data[5]]) return false;
    *out = (MsgType)data[5];
    return true;
}

bool circle_wire::decode(const uint8_t *data, size_t len, Beacon *out)
{
    MsgType t;
    if (!type(data, len, &t) || t != MsgType::BEACON) return false;
    if (data[6] > (uint8_t)Phase::READY || data[7] < 1 || data[7] > SessionCore::SESSIONS_PER_SET || data[9] > 100)
        return false;
    out->phase = (Phase)data[6];
    out->index = data[7];
    out->focused = (data[8] & FLAG_FOCUSED) != 0;
    out->pct = data[9];
    out->seq = (uint16_t)(data[10] | (data[11] << 8));
    out->group = get32(data + 12);
    // The name field may be missing its terminator, or be cut mid-character by a bad sender
    char name[12];
    memcpy(name, data + 16, 11);
    name[11] = '\0';
    copy_utf8(out->name, sizeof(out->name), name);
    return true;
}

bool circle_wire::decode(const uint8_t *data, size_t len, Invite *out)
{
    MsgType t;
    if (!type(data, len, &t) || t != MsgType::INVITE) return false;
    memcpy(out->to, data + 6, 6);
    out->group = get32(data + 12);
    out->study = data[16];
    out->brk = data[17];
    out->test = (data[18] & FLAG_TEST) != 0;
    return out->group != 0 && out->study > 0 && out->brk > 0;
}

bool circle_wire::decode(const uint8_t *data, size_t len, Decline *out)
{
    MsgType t;
    if (!type(data, len, &t) || t != MsgType::DECLINE) return false;
    memcpy(out->to, data + 6, 6);
    out->group = get32(data + 12);
    return out->group != 0;
}

// ---- Friends ----

void CircleCore::setMac(const uint8_t mac[6])
{
    memcpy(mac_, mac, 6);
}

int CircleCore::find(const uint8_t mac[6]) const
{
    for (int i = 0; i < count_; i++)
        if (memcmp(peers_[i].mac, mac, 6) == 0) return i;
    return -1;
}

CircleCore::Invitee *CircleCore::invitee(const uint8_t mac[6])
{
    for (int i = 0; i < invitee_count_; i++)
        if (memcmp(invitees_[i].mac, mac, 6) == 0) return &invitees_[i];
    return nullptr;
}

bool CircleCore::onBeacon(const uint8_t mac[6], const Beacon &b, uint32_t now_ms)
{
    bool changed;
    int i = find(mac);
    if (i >= 0) {
        const Beacon &old = peers_[i].beacon;
        changed = strcmp(old.name, b.name) != 0 || old.phase != b.phase || old.index != b.index ||
                  old.focused != b.focused || old.pct != b.pct || old.group != b.group;
    } else {
        if (count_ >= MAX_PEERS) return false;
        i = count_++;
        memcpy(peers_[i].mac, mac, 6);
        changed = true;
    }
    peers_[i].beacon = b;
    peers_[i].last_heard_ms = now_ms;
    if (changed) changed_ = true;

    const bool from_host = memcmp(mac, host_, 6) == 0;
    switch (role_) {
        case GroupRole::HOSTING: {
            Invitee *v = invitee(mac);
            if (v && v->status == FriendStatus::INVITED && b.group == group_) {
                v->status = FriendStatus::MEMBER;   // accepted
                changed_ = true;
            } else if (v && v->status == FriendStatus::MEMBER && b.group != group_) {
                *v = invitees_[--invitee_count_];   // left before the start
                changed_ = true;
            }
            break;
        }
        case GroupRole::INVITED:
            // The host closed the group, or started without this device
            if (from_host && (b.group != invitation_.group || b.studying())) {
                role_ = GroupRole::NONE;
                changed_ = true;
            }
            break;
        case GroupRole::JOINED:
            if (!from_host) break;
            if (b.group != group_) {
                leaveGroup();
            } else if (b.studying() && !started_) {
                setAction(GroupAction::START, invitation_);
                started_ = true;
            }
            break;
        case GroupRole::IN_SET:
            // A member started the next session: follow
            if (isMember(b) && b.studying() && (phase_ == Phase::BREAK || phase_ == Phase::READY) &&
                b.index == index_ + 1 && next_for_ != b.index) {
                setAction(GroupAction::NEXT, invitation_);
                next_for_ = b.index;
            }
            break;
        default:
            break;
    }
    return changed;
}

void CircleCore::onInvite(const uint8_t from[6], const Invite &m, uint32_t now_ms)
{
    if (memcmp(m.to, mac_, 6) != 0 || m.group == 0) return;
    if (find(from) < 0) return; // not heard from yet; the invite is resent
    // A resend of the invite already on screen, or already accepted
    if (role_ != GroupRole::NONE && role_ != GroupRole::HOSTING && memcmp(host_, from, 6) == 0 &&
        invitation_.group == m.group)
        return;
    if (phase_ != Phase::IDLE || role_ != GroupRole::NONE) {
        queueDecline(from, m.group); // busy
        return;
    }
    role_ = GroupRole::INVITED;
    invitation_ = m;
    memcpy(host_, from, 6);
    invite_until_ms_ = now_ms + INVITE_TIMEOUT_MS;
    changed_ = true;
}

void CircleCore::onDecline(const uint8_t from[6], const Decline &m, uint32_t now_ms)
{
    if (memcmp(m.to, mac_, 6) != 0 || role_ != GroupRole::HOSTING || m.group != group_) return;
    Invitee *v = invitee(from);
    if (!v || v->status != FriendStatus::INVITED) return;
    v->status = FriendStatus::DECLINED;
    v->until_ms = now_ms + DECLINED_SHOWN_MS;
    changed_ = true;
}

// ---- This device ----

void CircleCore::setSelf(Phase phase, int index)
{
    const Phase was = phase_;
    phase_ = phase;
    index_ = index;
    if (phase == Phase::STUDY) started_ = false;

    if (was != Phase::IDLE && phase == Phase::IDLE) {
        // The set is over (ended, or the long rest ran out)
        if (role_ == GroupRole::IN_SET) leaveGroup();
    } else if (was == Phase::IDLE && phase != Phase::IDLE) {
        switch (role_) {
            case GroupRole::HOSTING: {
                // Started: whoever joined is in; unanswered invites are void
                role_ = GroupRole::IN_SET;
                int kept = 0;
                for (int i = 0; i < invitee_count_; i++)
                    if (invitees_[i].status == FriendStatus::DECLINED) invitees_[kept++] = invitees_[i];
                invitee_count_ = kept;
                changed_ = true;
                break;
            }
            case GroupRole::JOINED:
                role_ = GroupRole::IN_SET;
                changed_ = true;
                break;
            case GroupRole::INVITED:
                // Started a session of their own instead
                queueDecline(host_, invitation_.group);
                role_ = GroupRole::NONE;
                changed_ = true;
                break;
            default:
                break;
        }
    }
}

void CircleCore::leaveGroup()
{
    role_ = GroupRole::NONE;
    group_ = 0;
    started_ = false;
    next_for_ = 0;
    // "Declined" stays on screen a little longer
    int kept = 0;
    for (int i = 0; i < invitee_count_; i++)
        if (invitees_[i].status == FriendStatus::DECLINED) invitees_[kept++] = invitees_[i];
    invitee_count_ = kept;
    changed_ = true;
}

void CircleCore::queueDecline(const uint8_t to[6], uint32_t group)
{
    for (int i = 0; i < decline_count_; i++)
        if (memcmp(declines_[i].msg.to, to, 6) == 0 && declines_[i].msg.group == group) return;
    if (decline_count_ >= MAX_DECLINES) return;
    PendingDecline &d = declines_[decline_count_++];
    memcpy(d.msg.to, to, 6);
    d.msg.group = group;
    d.sends_left = DECLINE_SENDS;
    d.next_send_ms = 0;
}

void CircleCore::setAction(GroupAction::Type type, const Invite &lengths)
{
    action_.type = type;
    action_.study = lengths.study;
    action_.brk = lengths.brk;
    action_.test = lengths.test;
}

bool CircleCore::invite(const uint8_t mac[6], int study, int brk, bool test, uint32_t new_group, uint32_t now_ms)
{
    if (!canInvite()) return false;
    const int i = find(mac);
    if (i < 0) return false;
    const FriendStatus s = status(i);
    if (s != FriendStatus::FREE && s != FriendStatus::DECLINED) return false;

    Invitee *v = invitee(mac);
    if (!v) {
        if (invitee_count_ >= MAX_INVITEES) {
            // canInvite() means a slot holds an old "Declined": reuse it
            for (int k = 0; k < invitee_count_ && !v; k++)
                if (invitees_[k].status == FriendStatus::DECLINED) v = &invitees_[k];
            if (!v) return false;
        } else {
            v = &invitees_[invitee_count_++];
        }
    }
    if (role_ == GroupRole::NONE) {
        if (new_group == 0) return false;
        role_ = GroupRole::HOSTING;
        group_ = new_group;
        invitation_ = {};
        invitation_.group = new_group;
        invitation_.study = (uint8_t)study;
        invitation_.brk = (uint8_t)brk;
        invitation_.test = test;
    }
    memcpy(v->mac, mac, 6);
    v->status = FriendStatus::INVITED;
    v->until_ms = now_ms + INVITE_TIMEOUT_MS;
    v->next_send_ms = now_ms;
    changed_ = true;
    return true;
}

bool CircleCore::startHosted()
{
    if (!canStart() || started_) return false;
    setAction(GroupAction::START, invitation_);
    started_ = true;
    return true;
}

bool CircleCore::cancelHosted()
{
    if (role_ != GroupRole::HOSTING) return false;
    leaveGroup();
    invitee_count_ = 0;
    return true;
}

bool CircleCore::accept()
{
    if (role_ != GroupRole::INVITED || phase_ != Phase::IDLE) return false;
    role_ = GroupRole::JOINED;
    group_ = invitation_.group;
    changed_ = true;
    return true;
}

bool CircleCore::decline()
{
    if (role_ == GroupRole::INVITED) {
        queueDecline(host_, invitation_.group);
        role_ = GroupRole::NONE;
        changed_ = true;
        return true;
    }
    if (role_ == GroupRole::JOINED) {
        leaveGroup();
        return true;
    }
    return false;
}

// ---- Time ----

bool CircleCore::expire(uint32_t now_ms)
{
    bool dropped = false;
    int kept = 0;
    for (int i = 0; i < count_; i++) {
        if (now_ms - peers_[i].last_heard_ms > PEER_TIMEOUT_MS) {
            dropped = true;
            continue;
        }
        if (kept != i) peers_[kept] = peers_[i];
        kept++;
    }
    count_ = kept;
    if (dropped) changed_ = true;
    return dropped;
}

void CircleCore::tick(uint32_t now_ms)
{
    expire(now_ms);

    // A guest whose host went quiet, or whose invite ran out
    if (role_ == GroupRole::INVITED && (find(host_) < 0 || reached(now_ms, invite_until_ms_))) {
        role_ = GroupRole::NONE;
        changed_ = true;
    }
    if (role_ == GroupRole::JOINED && find(host_) < 0) leaveGroup();

    // Invites running out, friends who joined and went quiet, old "Declined"s
    int kept = 0;
    for (int i = 0; i < invitee_count_; i++) {
        const Invitee &v = invitees_[i];
        const bool gone = (v.status == FriendStatus::MEMBER && find(v.mac) < 0) ||
                          (v.status != FriendStatus::MEMBER && reached(now_ms, v.until_ms));
        if (gone) {
            changed_ = true;
            continue;
        }
        invitees_[kept++] = v;
    }
    invitee_count_ = kept;

    if (role_ == GroupRole::HOSTING) {
        int waiting = 0, joined = 0;
        for (int i = 0; i < invitee_count_; i++) {
            if (invitees_[i].status == FriendStatus::INVITED) waiting++;
            if (invitees_[i].status == FriendStatus::MEMBER) joined++;
        }
        if (waiting == 0 && joined == 0) leaveGroup();   // nobody coming
        else if (waiting == 0) startHosted();             // everyone answered
    }
}

// ---- Out ----

bool CircleCore::nextMessage(uint32_t now_ms, uint8_t *out, size_t *len)
{
    if (role_ == GroupRole::HOSTING) {
        for (int i = 0; i < invitee_count_; i++) {
            Invitee &v = invitees_[i];
            if (v.status != FriendStatus::INVITED || !reached(now_ms, v.next_send_ms)) continue;
            Invite m = invitation_;
            memcpy(m.to, v.mac, 6);
            v.next_send_ms = now_ms + INVITE_RESEND_MS;
            *len = circle_wire::encode(m, out);
            return true;
        }
    }
    for (int i = 0; i < decline_count_; i++) {
        PendingDecline &d = declines_[i];
        if (!reached(now_ms, d.next_send_ms)) continue;
        *len = circle_wire::encode(d.msg, out);
        d.next_send_ms = now_ms + INVITE_RESEND_MS / 2;
        if (--d.sends_left <= 0) declines_[i] = declines_[--decline_count_];
        return true;
    }
    return false;
}

GroupAction CircleCore::takeAction()
{
    const GroupAction a = action_;
    action_ = {};
    return a;
}

bool CircleCore::takeChanged()
{
    const bool c = changed_;
    changed_ = false;
    return c;
}

// ---- Views ----

FriendStatus CircleCore::status(int i) const
{
    const CirclePeer &p = peers_[i];
    if (isMember(p.beacon)) return FriendStatus::MEMBER;
    for (int k = 0; k < invitee_count_; k++) {
        if (memcmp(invitees_[k].mac, p.mac, 6) != 0) continue;
        if (invitees_[k].status == FriendStatus::INVITED) return FriendStatus::INVITED;
        if (invitees_[k].status == FriendStatus::DECLINED) return FriendStatus::DECLINED;
    }
    if (p.beacon.inSet() || p.beacon.group != 0) return FriendStatus::BUSY;
    return FriendStatus::FREE;
}

int CircleCore::membersStudying() const
{
    int n = 0;
    for (int i = 0; i < count_; i++)
        if (isMember(peers_[i].beacon) && peers_[i].beacon.studying()) n++;
    return n;
}

bool CircleCore::memberDrifting() const
{
    for (int i = 0; i < count_; i++) {
        const Beacon &b = peers_[i].beacon;
        if (isMember(b) && b.studying() && !b.focused) return true;
    }
    return false;
}

bool CircleCore::canInvite() const
{
    if (phase_ != Phase::IDLE) return false;
    if (role_ == GroupRole::NONE) return true;
    if (role_ != GroupRole::HOSTING) return false;
    int active = 0;
    for (int i = 0; i < invitee_count_; i++)
        if (invitees_[i].status != FriendStatus::DECLINED) active++;
    return active < MAX_INVITEES;
}

bool CircleCore::canStart() const
{
    if (role_ != GroupRole::HOSTING) return false;
    for (int i = 0; i < invitee_count_; i++)
        if (invitees_[i].status == FriendStatus::MEMBER) return true;
    return false;
}

const char *CircleCore::hostName() const
{
    const int i = find(host_);
    return i >= 0 ? peers_[i].beacon.name : "";
}
