// Host test for the study circle rules (main/app/circle_core.cpp). No ESP-IDF needed:
//   g++ -std=c++17 -Wall -I main test/circle_test.cpp main/app/circle_core.cpp -o circle_test && ./circle_test

#include <stdio.h>
#include <string.h>

#include <vector>

#include "app/circle_core.hpp"

static int failures = 0;

#define CHECK_EQ(actual, expected)                                                        \
    do {                                                                                  \
        const long a_ = (long)(actual), e_ = (long)(expected);                            \
        if (a_ != e_) {                                                                   \
            printf("  FAIL %s:%d  %s == %ld, expected %ld\n", __FILE__, __LINE__, #actual, a_, e_); \
            failures++;                                                                   \
        }                                                                                 \
    } while (0)

#define CHECK_STR(actual, expected)                                                       \
    do {                                                                                  \
        if (strcmp((actual), (expected)) != 0) {                                          \
            printf("  FAIL %s:%d  %s == \"%s\", expected \"%s\"\n", __FILE__, __LINE__,   \
                   #actual, (actual), (expected));                                        \
            failures++;                                                                   \
        }                                                                                 \
    } while (0)

static const uint8_t AVA[6] = {0x24, 0x6F, 0x28, 0x00, 0x00, 0x01};
static const uint8_t DEV[6] = {0x24, 0x6F, 0x28, 0x00, 0x00, 0x02};

static Beacon make(const char *name, bool studying, bool focused, int pct = 0, uint32_t group = 0)
{
    Beacon b = {};
    strcpy(b.name, name);
    b.phase = studying ? Phase::STUDY : Phase::IDLE;
    b.index = 1;
    b.focused = focused;
    b.pct = (uint8_t)pct;
    b.group = group;
    return b;
}

static void test_wire()
{
    puts("wire format");
    Beacon b = make("Ava", true, false, 87, 0xA1B2C3D4);
    b.phase = Phase::READY;
    b.index = 3;
    b.seq = 0x1234;
    uint8_t buf[circle_wire::MAX_SIZE];
    CHECK_EQ(circle_wire::encode(b, buf), 27);
    CHECK_EQ(memcmp(buf, "POMO", 4), 0);
    CHECK_EQ(buf[4], 2);
    CHECK_EQ(buf[5], 0);
    CHECK_EQ(buf[10], 0x34);
    CHECK_EQ(buf[11], 0x12);
    CHECK_EQ(buf[12], 0xD4);

    MsgType t;
    CHECK_EQ(circle_wire::type(buf, 27, &t), true);
    CHECK_EQ((int)t, (int)MsgType::BEACON);
    Beacon d = {};
    CHECK_EQ(circle_wire::decode(buf, 27, &d), true);
    CHECK_STR(d.name, "Ava");
    CHECK_EQ((int)d.phase, (int)Phase::READY);
    CHECK_EQ(d.index, 3);
    CHECK_EQ(d.focused, false);
    CHECK_EQ(d.pct, 87);
    CHECK_EQ(d.seq, 0x1234);
    CHECK_EQ(d.group, 0xA1B2C3D4);

    // Not ours: short, wrong magic, the old version 1, bad phase, index or %, unknown type
    CHECK_EQ(circle_wire::decode(buf, 26, &d), false);
    uint8_t bad[circle_wire::MAX_SIZE];
    memcpy(bad, buf, sizeof(bad)); bad[0] = 'X';
    CHECK_EQ(circle_wire::decode(bad, 27, &d), false);
    memcpy(bad, buf, sizeof(bad)); bad[4] = 1;
    CHECK_EQ(circle_wire::decode(bad, 27, &d), false);
    memcpy(bad, buf, sizeof(bad)); bad[6] = 5;
    CHECK_EQ(circle_wire::decode(bad, 27, &d), false);
    memcpy(bad, buf, sizeof(bad)); bad[7] = 0;
    CHECK_EQ(circle_wire::decode(bad, 27, &d), false);
    memcpy(bad, buf, sizeof(bad)); bad[9] = 101;
    CHECK_EQ(circle_wire::decode(bad, 27, &d), false);
    memcpy(bad, buf, sizeof(bad)); bad[5] = 3;
    CHECK_EQ(circle_wire::type(bad, 27, &t), false);
    CHECK_EQ(circle_wire::decode(nullptr, 27, &d), false);

    // A longer packet (a later version adding fields at the end) still decodes
    uint8_t longer[40] = {};
    memcpy(longer, buf, 27);
    CHECK_EQ(circle_wire::decode(longer, sizeof(longer), &d), true);

    // A name with no terminator, or cut mid-character, comes out clean
    memcpy(bad, buf, sizeof(bad));
    memcpy(bad + 16, "ABCDEFGHIJK", 11);
    CHECK_EQ(circle_wire::decode(bad, 27, &d), true);
    CHECK_STR(d.name, "ABCDEFGHIJ");
    memcpy(bad + 16, "Zoe\xC3\xAB\xC3\xAB\xC3\xAB\xC3", 11);
    circle_wire::decode(bad, 27, &d);
    CHECK_STR(d.name, "Zoe\xC3\xAB\xC3\xAB\xC3\xAB");

    // Invite and decline
    Invite inv = {};
    memcpy(inv.to, DEV, 6);
    inv.group = 77;
    inv.study = 45;
    inv.brk = 9;
    inv.test = true;
    CHECK_EQ(circle_wire::encode(inv, buf), 19);
    CHECK_EQ(circle_wire::type(buf, 19, &t), true);
    CHECK_EQ((int)t, (int)MsgType::INVITE);
    CHECK_EQ(circle_wire::decode(buf, 19, &d), false);      // not a beacon
    Invite inv2 = {};
    CHECK_EQ(circle_wire::decode(buf, 19, &inv2), true);
    CHECK_EQ(memcmp(inv2.to, DEV, 6), 0);
    CHECK_EQ(inv2.group, 77);
    CHECK_EQ(inv2.study, 45);
    CHECK_EQ(inv2.brk, 9);
    CHECK_EQ(inv2.test, true);
    CHECK_EQ(circle_wire::decode(buf, 18, &inv2), false);

    Decline dec = {};
    memcpy(dec.to, AVA, 6);
    dec.group = 77;
    CHECK_EQ(circle_wire::encode(dec, buf), 16);
    Decline dec2 = {};
    CHECK_EQ(circle_wire::decode(buf, 16, &dec2), true);
    CHECK_EQ(memcmp(dec2.to, AVA, 6), 0);
    CHECK_EQ(dec2.group, 77);
}

static void test_join_and_change()
{
    puts("join, refresh, change");
    CircleCore c;
    CHECK_EQ(c.onBeacon(AVA, make("Ava", true, true), 1000), true);    // joined
    CHECK_EQ(c.count(), 1);
    CHECK_EQ(c.onBeacon(AVA, make("Ava", true, true), 2500), false);   // same news
    CHECK_EQ(c.peer(0).last_heard_ms, 2500);
    CHECK_EQ(c.onBeacon(AVA, make("Ava", true, false), 4000), true);   // drifting now
    CHECK_EQ(c.onBeacon(AVA, make("Ava", false, false, 64), 5500), true);
    CHECK_EQ(c.onBeacon(AVA, make("Ava R", false, false, 64), 7000), true);
    Beacon seq_only = make("Ava R", false, false, 64);
    seq_only.seq = 99;
    CHECK_EQ(c.onBeacon(AVA, seq_only, 8500), false);                  // seq alone isn't news
    CHECK_EQ(c.onBeacon(DEV, make("Dev", false, true), 8500), true);
    CHECK_EQ(c.count(), 2);
}

static void test_timeout()
{
    puts("leave after 6 s of silence");
    CircleCore c;
    c.onBeacon(AVA, make("Ava", true, true), 1000);
    c.onBeacon(DEV, make("Dev", true, true), 3000);
    CHECK_EQ(c.expire(7000), false);    // Ava: exactly 6 s, still in
    CHECK_EQ(c.expire(7001), true);     // Ava gone
    CHECK_EQ(c.count(), 1);
    CHECK_STR(c.peer(0).beacon.name, "Dev");
    CHECK_EQ(c.expire(9000), false);
    CHECK_EQ(c.expire(9001), true);
    CHECK_EQ(c.count(), 0);

    // Back again later: joins as new
    CHECK_EQ(c.onBeacon(AVA, make("Ava", true, true), 20000), true);

    // The ms clock wrapping around doesn't drop anyone early
    CircleCore w;
    w.onBeacon(AVA, make("Ava", true, true), 0xFFFFF000u);
    CHECK_EQ(w.expire(0x00000100u), false);   // 4.3 s later, across the wrap
    CHECK_EQ(w.expire(0x00001000u), true);    // 8.2 s later
}

static void test_full_circle()
{
    puts("at most 4 friends");
    CircleCore c;
    uint8_t mac[6] = {0x24, 0x6F, 0x28, 0, 0, 0};
    for (int i = 0; i < 5; i++) {
        mac[5] = (uint8_t)(10 + i);
        c.onBeacon(mac, make("F", true, true), 1000);
    }
    CHECK_EQ(c.count(), 4);
    mac[5] = 14;
    CHECK_EQ(c.onBeacon(mac, make("F", true, true), 1500), false);  // the fifth is still out
    CHECK_EQ(c.count(), 4);
    // Once one leaves, the fifth can join
    mac[5] = 10;
    for (int i = 1; i < 4; i++) {
        mac[5] = (uint8_t)(10 + i);
        c.onBeacon(mac, make("F", true, true), 6000);
    }
    c.expire(7500);   // the first one (last heard at 1000) leaves
    CHECK_EQ(c.count(), 3);
    mac[5] = 14;
    CHECK_EQ(c.onBeacon(mac, make("F", true, true), 7500), true);
    CHECK_EQ(c.count(), 4);
}

// ---- Study groups: two or three cores talking through encoded messages ----

static const uint8_t HOST[6] = {0x24, 0x6F, 0x28, 0x00, 0x00, 0x10};

typedef std::vector<std::vector<uint8_t>> Packets;

// Everything `from` has to send at `now`
static Packets drain(CircleCore &from, uint32_t now)
{
    Packets out;
    uint8_t buf[circle_wire::MAX_SIZE];
    size_t len = 0;
    while (from.nextMessage(now, buf, &len)) out.emplace_back(buf, buf + len);
    return out;
}

// Broadcast: every core in range hears every packet
static void deliver(const Packets &packets, const uint8_t from[6], CircleCore &to, uint32_t now)
{
    for (const auto &p : packets) {
        MsgType t;
        if (!circle_wire::type(p.data(), p.size(), &t)) continue;
        if (t == MsgType::INVITE) {
            Invite m;
            if (circle_wire::decode(p.data(), p.size(), &m)) to.onInvite(from, m, now);
        } else if (t == MsgType::DECLINE) {
            Decline m;
            if (circle_wire::decode(p.data(), p.size(), &m)) to.onDecline(from, m, now);
        }
    }
}

static Beacon self(const char *name, Phase phase, int index, uint32_t group, bool focused = true)
{
    Beacon b = make(name, false, focused, 0, group);
    b.phase = phase;
    b.index = (uint8_t)index;
    return b;
}

static void meet(CircleCore &a, const uint8_t a_mac[6], const char *a_name, CircleCore &b,
                 const uint8_t b_mac[6], const char *b_name, uint32_t now)
{
    a.onBeacon(b_mac, self(b_name, Phase::IDLE, 1, b.group()), now);
    b.onBeacon(a_mac, self(a_name, Phase::IDLE, 1, a.group()), now);
}

static void test_invite_accept_start()
{
    puts("invite, accept, start together");
    CircleCore h, g;
    h.setMac(HOST);
    g.setMac(AVA);
    meet(h, HOST, "Ibrahim", g, AVA, "Ava", 0);
    CHECK_EQ((int)h.status(0), (int)FriendStatus::FREE);
    CHECK_EQ(h.canInvite(), true);
    CHECK_EQ(h.membersStudying(), 0);

    CHECK_EQ(h.invite(AVA, 25, 5, false, 42, 100), true);
    CHECK_EQ((int)h.role(), (int)GroupRole::HOSTING);
    CHECK_EQ(h.group(), 42);
    CHECK_EQ((int)h.status(0), (int)FriendStatus::INVITED);
    CHECK_EQ(h.canStart(), false);
    CHECK_EQ(h.invite(AVA, 25, 5, false, 43, 100), false);     // already invited

    // Sent now, then resent every second
    Packets sent = drain(h, 100);
    CHECK_EQ(sent.size(), 1);
    CHECK_EQ(drain(h, 600).size(), 0);
    CHECK_EQ(drain(h, 1100).size(), 1);

    g.onBeacon(HOST, self("Ibrahim", Phase::IDLE, 1, 42), 150);
    g.takeChanged();
    deliver(sent, HOST, g, 200);
    CHECK_EQ((int)g.role(), (int)GroupRole::INVITED);
    CHECK_EQ(g.takeChanged(), true);
    CHECK_STR(g.hostName(), "Ibrahim");
    CHECK_EQ(g.invitation().study, 25);
    CHECK_EQ(g.invitation().brk, 5);
    CHECK_EQ(g.group(), 0);                 // not in it until accepted
    deliver(sent, HOST, g, 1200);           // the resend changes nothing
    CHECK_EQ(g.takeChanged(), false);

    CHECK_EQ(g.accept(), true);
    CHECK_EQ((int)g.role(), (int)GroupRole::JOINED);
    CHECK_EQ(g.group(), 42);

    // The host sees Ava in the group: everyone answered, so it starts
    h.onBeacon(AVA, self("Ava", Phase::IDLE, 1, 42), 1500);
    CHECK_EQ((int)h.status(0), (int)FriendStatus::MEMBER);
    CHECK_EQ(h.canStart(), true);
    CHECK_EQ(drain(h, 2200).size(), 0);     // no more invites to send
    h.tick(1600);
    GroupAction a = h.takeAction();
    CHECK_EQ((int)a.type, (int)GroupAction::START);
    CHECK_EQ(a.study, 25);
    CHECK_EQ(a.brk, 5);
    h.tick(1700);
    CHECK_EQ((int)h.takeAction().type, (int)GroupAction::NONE);   // once
    h.setSelf(Phase::STUDY, 1);
    CHECK_EQ((int)h.role(), (int)GroupRole::IN_SET);

    // Ava follows when the host's beacon shows it studying
    g.onBeacon(HOST, self("Ibrahim", Phase::STUDY, 1, 42), 2000);
    a = g.takeAction();
    CHECK_EQ((int)a.type, (int)GroupAction::START);
    CHECK_EQ(a.study, 25);
    g.onBeacon(HOST, self("Ibrahim", Phase::STUDY, 1, 42), 3500);
    CHECK_EQ((int)g.takeAction().type, (int)GroupAction::NONE);
    g.setSelf(Phase::STUDY, 1);
    CHECK_EQ((int)g.role(), (int)GroupRole::IN_SET);

    // Both studying: each counts the other; a drift pauses the bonus
    h.onBeacon(AVA, self("Ava", Phase::STUDY, 1, 42), 3500);
    CHECK_EQ(h.membersStudying(), 1);
    CHECK_EQ(g.membersStudying(), 1);
    CHECK_EQ(h.memberDrifting(), false);
    h.onBeacon(AVA, self("Ava", Phase::STUDY, 1, 42, false), 5000);
    CHECK_EQ(h.memberDrifting(), true);
    CHECK_EQ(h.canInvite(), false);         // nobody joins once it's running
}

static void test_nearby_is_not_enough()
{
    puts("friends nearby but not in the group don't count");
    CircleCore c;
    c.setMac(HOST);
    c.onBeacon(AVA, make("Ava", true, true), 1000);
    c.onBeacon(DEV, make("Dev", true, false, 0, 99), 1000);     // studying in another group
    c.setSelf(Phase::STUDY, 1);
    CHECK_EQ(c.membersStudying(), 0);
    CHECK_EQ(c.memberDrifting(), false);
    CHECK_EQ((int)c.status(0), (int)FriendStatus::BUSY);
    CHECK_EQ((int)c.status(1), (int)FriendStatus::BUSY);
}

static void test_two_guests()
{
    puts("two invites: wait for both answers");
    CircleCore h, a, d;
    h.setMac(HOST);
    a.setMac(AVA);
    d.setMac(DEV);
    meet(h, HOST, "Ibrahim", a, AVA, "Ava", 0);
    meet(h, HOST, "Ibrahim", d, DEV, "Dev", 0);
    CHECK_EQ(h.invite(AVA, 45, 9, true, 7, 0), true);
    CHECK_EQ(h.invite(DEV, 15, 3, false, 8, 0), true);  // same group and lengths as the first
    CHECK_EQ(h.group(), 7);
    CHECK_EQ(h.canInvite(), false);                     // two guests is the most

    Packets sent = drain(h, 0);
    CHECK_EQ(sent.size(), 2);
    a.onBeacon(HOST, self("Ibrahim", Phase::IDLE, 1, 7), 10);
    d.onBeacon(HOST, self("Ibrahim", Phase::IDLE, 1, 7), 10);
    deliver(sent, HOST, a, 20);
    deliver(sent, HOST, d, 20);
    CHECK_EQ((int)a.role(), (int)GroupRole::INVITED);
    CHECK_EQ((int)d.role(), (int)GroupRole::INVITED);
    CHECK_EQ(d.invitation().study, 45);
    CHECK_EQ(d.invitation().test, true);

    // Ava joins; Dev hasn't answered, so no start yet
    a.accept();
    h.onBeacon(AVA, self("Ava", Phase::IDLE, 1, 7), 500);
    h.tick(600);
    CHECK_EQ((int)h.takeAction().type, (int)GroupAction::NONE);
    CHECK_EQ(h.canStart(), true);

    // Dev says no, three times over for the radio's sake; then the host starts
    CHECK_EQ(d.decline(), true);
    CHECK_EQ((int)d.role(), (int)GroupRole::NONE);
    Packets no = drain(d, 1000);
    CHECK_EQ(no.size(), 1);
    CHECK_EQ(drain(d, 1200).size(), 0);
    CHECK_EQ(drain(d, 1500).size(), 1);
    CHECK_EQ(drain(d, 2000).size(), 1);
    CHECK_EQ(drain(d, 3000).size(), 0);
    deliver(no, DEV, h, 1000);
    CHECK_EQ((int)h.status(1), (int)FriendStatus::DECLINED);
    h.tick(1100);
    CHECK_EQ((int)h.takeAction().type, (int)GroupAction::START);

    // "Declined" shows for a while after the start, then goes
    h.setSelf(Phase::STUDY, 1);
    CHECK_EQ((int)h.status(1), (int)FriendStatus::DECLINED);
    for (uint32_t t = 2000; t <= 12000; t += 1000) {
        h.onBeacon(AVA, self("Ava", Phase::STUDY, 1, 7), t);
        h.onBeacon(DEV, self("Dev", Phase::IDLE, 1, 0), t);
        h.tick(t);
    }
    CHECK_EQ((int)h.status(1), (int)FriendStatus::FREE);
}

static void test_start_button()
{
    puts("the host starts without waiting");
    CircleCore h;
    h.setMac(HOST);
    h.onBeacon(AVA, make("Ava", false, true), 0);
    h.onBeacon(DEV, make("Dev", false, true), 0);
    CHECK_EQ(h.startHosted(), false);           // not hosting
    h.invite(AVA, 25, 5, false, 5, 0);
    h.invite(DEV, 25, 5, false, 5, 0);
    CHECK_EQ(h.startHosted(), false);           // nobody joined yet
    h.onBeacon(AVA, make("Ava", false, true, 0, 5), 500);
    CHECK_EQ(h.startHosted(), true);            // Dev still thinking
    CHECK_EQ((int)h.takeAction().type, (int)GroupAction::START);
    h.setSelf(Phase::STUDY, 1);
    CHECK_EQ((int)h.status(1), (int)FriendStatus::FREE);   // Dev's invite is void
    CHECK_EQ(drain(h, 1500).size(), 0);
}

static void test_timeouts()
{
    puts("invites run out after 30 s");
    CircleCore h, g;
    h.setMac(HOST);
    g.setMac(AVA);
    meet(h, HOST, "Ibrahim", g, AVA, "Ava", 0);
    h.invite(AVA, 25, 5, false, 3, 0);
    g.onBeacon(HOST, self("Ibrahim", Phase::IDLE, 1, 3), 0);
    deliver(drain(h, 0), HOST, g, 0);
    CHECK_EQ((int)g.role(), (int)GroupRole::INVITED);
    for (uint32_t t = 1000; t < 30000; t += 1000) {
        h.onBeacon(AVA, self("Ava", Phase::IDLE, 1, 0), t);
        g.onBeacon(HOST, self("Ibrahim", Phase::IDLE, 1, 3), t);
        h.tick(t);
        g.tick(t);
    }
    CHECK_EQ((int)h.role(), (int)GroupRole::HOSTING);
    CHECK_EQ((int)g.role(), (int)GroupRole::INVITED);
    h.onBeacon(AVA, self("Ava", Phase::IDLE, 1, 0), 30000);
    g.onBeacon(HOST, self("Ibrahim", Phase::IDLE, 1, 3), 30000);
    h.tick(30000);
    g.tick(30000);
    CHECK_EQ((int)h.role(), (int)GroupRole::NONE);      // nobody coming: the group closes
    CHECK_EQ(h.group(), 0);
    CHECK_EQ((int)h.status(0), (int)FriendStatus::FREE);
    CHECK_EQ((int)g.role(), (int)GroupRole::NONE);
    CHECK_EQ(h.canInvite(), true);

    puts("a host that goes quiet");
    CircleCore j;
    j.setMac(AVA);
    j.onBeacon(HOST, self("Ibrahim", Phase::IDLE, 1, 9), 0);
    Invite inv = {};
    memcpy(inv.to, AVA, 6);
    inv.group = 9;
    inv.study = 25;
    inv.brk = 5;
    j.onInvite(HOST, inv, 0);
    j.accept();
    j.tick(6000);
    CHECK_EQ((int)j.role(), (int)GroupRole::JOINED);
    j.tick(6001);
    CHECK_EQ((int)j.role(), (int)GroupRole::NONE);
    CHECK_EQ(j.group(), 0);

    // An invite from a device not heard yet waits for the resend
    CircleCore k;
    k.setMac(AVA);
    k.onInvite(HOST, inv, 0);
    CHECK_EQ((int)k.role(), (int)GroupRole::NONE);
    Invite other = inv;
    memcpy(other.to, DEV, 6);              // and one for someone else is ignored
    k.onBeacon(HOST, self("Ibrahim", Phase::IDLE, 1, 9), 100);
    k.onInvite(HOST, other, 100);
    CHECK_EQ((int)k.role(), (int)GroupRole::NONE);
}

static void test_busy()
{
    puts("busy friends can't be invited, and say no");
    CircleCore h;
    h.setMac(HOST);
    h.onBeacon(AVA, make("Ava", true, true), 0);            // studying
    CHECK_EQ((int)h.status(0), (int)FriendStatus::BUSY);
    CHECK_EQ(h.invite(AVA, 25, 5, false, 1, 0), false);
    h.setSelf(Phase::STUDY, 1);
    h.onBeacon(DEV, make("Dev", false, true), 0);
    CHECK_EQ(h.invite(DEV, 25, 5, false, 1, 0), false);     // can't host mid-session
    CHECK_EQ(h.canInvite(), false);

    // Invited while studying: a decline goes back
    Invite inv = {};
    memcpy(inv.to, HOST, 6);
    inv.group = 4;
    inv.study = 25;
    inv.brk = 5;
    h.onInvite(DEV, inv, 0);
    CHECK_EQ((int)h.role(), (int)GroupRole::NONE);
    Packets no = drain(h, 0);
    CHECK_EQ(no.size(), 1);
    Decline dec;
    CHECK_EQ(circle_wire::decode(no[0].data(), no[0].size(), &dec), true);
    CHECK_EQ(memcmp(dec.to, DEV, 6), 0);
    CHECK_EQ(dec.group, 4);

    // Invited, then started a session of their own: that's a no too
    CircleCore g;
    g.setMac(AVA);
    g.onBeacon(HOST, self("Ibrahim", Phase::IDLE, 1, 4), 0);
    memcpy(inv.to, AVA, 6);
    g.onInvite(HOST, inv, 0);
    CHECK_EQ((int)g.role(), (int)GroupRole::INVITED);
    g.setSelf(Phase::STUDY, 1);
    CHECK_EQ((int)g.role(), (int)GroupRole::NONE);
    CHECK_EQ(drain(g, 0).size(), 1);
}

static void test_cancel_and_leave()
{
    puts("host cancels, guest leaves");
    CircleCore h, g;
    h.setMac(HOST);
    g.setMac(AVA);
    meet(h, HOST, "Ibrahim", g, AVA, "Ava", 0);
    h.invite(AVA, 25, 5, false, 11, 0);
    g.onBeacon(HOST, self("Ibrahim", Phase::IDLE, 1, 11), 0);
    deliver(drain(h, 0), HOST, g, 0);
    g.accept();

    // The guest changes their mind before the start
    CHECK_EQ(g.decline(), true);
    CHECK_EQ(g.group(), 0);
    h.onBeacon(AVA, self("Ava", Phase::IDLE, 1, 11), 500);     // joined...
    h.onBeacon(AVA, self("Ava", Phase::IDLE, 1, 0), 2000);     // ...and left
    CHECK_EQ((int)h.status(0), (int)FriendStatus::FREE);
    h.tick(2100);
    CHECK_EQ((int)h.role(), (int)GroupRole::NONE);
    CHECK_EQ((int)h.takeAction().type, (int)GroupAction::NONE);

    // The host cancels: the guest waiting sees the group gone from its beacon
    h.invite(AVA, 25, 5, false, 12, 3000);
    g.onBeacon(HOST, self("Ibrahim", Phase::IDLE, 1, 12), 3000);
    deliver(drain(h, 3000), HOST, g, 3000);
    g.accept();
    CHECK_EQ(h.cancelHosted(), true);
    CHECK_EQ(h.group(), 0);
    CHECK_EQ(h.cancelHosted(), false);
    g.onBeacon(HOST, self("Ibrahim", Phase::IDLE, 1, 0), 4000);
    CHECK_EQ((int)g.role(), (int)GroupRole::NONE);
    CHECK_EQ(g.group(), 0);
}

static void test_whole_set()
{
    puts("the group stays in step for the set");
    CircleCore h, g;
    h.setMac(HOST);
    g.setMac(AVA);
    meet(h, HOST, "Ibrahim", g, AVA, "Ava", 0);
    h.invite(AVA, 25, 5, false, 21, 0);
    g.onBeacon(HOST, self("Ibrahim", Phase::IDLE, 1, 21), 0);
    deliver(drain(h, 0), HOST, g, 0);
    g.accept();
    h.onBeacon(AVA, self("Ava", Phase::IDLE, 1, 21), 100);
    h.tick(100);
    h.takeAction();
    h.setSelf(Phase::STUDY, 1);
    g.onBeacon(HOST, self("Ibrahim", Phase::STUDY, 1, 21), 200);
    g.takeAction();
    g.setSelf(Phase::STUDY, 1);

    // Session 1 over: Ava waits at READY, Ibrahim presses Start next
    g.setSelf(Phase::READY, 1);
    h.setSelf(Phase::STUDY, 2);
    g.onBeacon(HOST, self("Ibrahim", Phase::BREAK, 1, 21), 300);
    CHECK_EQ((int)g.takeAction().type, (int)GroupAction::NONE);
    g.onBeacon(HOST, self("Ibrahim", Phase::STUDY, 2, 21), 400);
    CHECK_EQ((int)g.takeAction().type, (int)GroupAction::NEXT);
    g.onBeacon(HOST, self("Ibrahim", Phase::STUDY, 2, 21), 1900);
    CHECK_EQ((int)g.takeAction().type, (int)GroupAction::NONE);    // once
    g.setSelf(Phase::STUDY, 2);

    // Mid-break, Ava taps Start now: Ibrahim follows
    h.setSelf(Phase::BREAK, 2);
    g.setSelf(Phase::STUDY, 3);
    h.onBeacon(AVA, self("Ava", Phase::STUDY, 3, 21), 2000);
    CHECK_EQ((int)h.takeAction().type, (int)GroupAction::NEXT);

    // Someone in another group starting session 3 doesn't count
    CircleCore x;
    x.setMac(DEV);
    x.onBeacon(HOST, self("Ibrahim", Phase::STUDY, 1, 21), 0);
    x.setSelf(Phase::READY, 2);
    x.onBeacon(HOST, self("Ibrahim", Phase::STUDY, 3, 21), 100);
    CHECK_EQ((int)x.takeAction().type, (int)GroupAction::NONE);

    // Ava ends the set: she leaves the group, and Ibrahim studies on alone
    h.setSelf(Phase::STUDY, 3);
    h.onBeacon(AVA, self("Ava", Phase::STUDY, 3, 21), 2500);
    CHECK_EQ(h.membersStudying(), 1);
    g.setSelf(Phase::IDLE, 1);
    CHECK_EQ((int)g.role(), (int)GroupRole::NONE);
    CHECK_EQ(g.group(), 0);
    h.onBeacon(AVA, self("Ava", Phase::IDLE, 1, 0), 3000);
    CHECK_EQ(h.membersStudying(), 0);
    CHECK_EQ((int)h.status(0), (int)FriendStatus::FREE);
    CHECK_EQ(h.group(), 21);        // still in its set

    // Ibrahim's set ends too
    h.setSelf(Phase::IDLE, 1);
    CHECK_EQ(h.group(), 0);
    CHECK_EQ((int)h.role(), (int)GroupRole::NONE);
}

int main()
{
    test_wire();
    test_join_and_change();
    test_timeout();
    test_full_circle();
    test_invite_accept_start();
    test_nearby_is_not_enough();
    test_two_guests();
    test_start_button();
    test_timeouts();
    test_busy();
    test_cancel_and_leave();
    test_whole_set();
    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    puts("all passed");
    return 0;
}
