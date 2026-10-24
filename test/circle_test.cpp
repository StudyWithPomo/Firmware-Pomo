// Host test for the study circle rules (main/app/circle_core.cpp). No ESP-IDF needed:
//   g++ -std=c++17 -Wall -I main test/circle_test.cpp main/app/circle_core.cpp -o circle_test && ./circle_test

#include <stdio.h>
#include <string.h>

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

static Beacon make(const char *name, bool in_session, bool focused, int pct = 0)
{
    Beacon b = {};
    strcpy(b.name, name);
    b.in_session = in_session;
    b.focused = focused;
    b.pct = (uint8_t)pct;
    return b;
}

static void test_wire()
{
    puts("beacon wire format");
    Beacon b = make("Ava", true, false, 87);
    b.seq = 0x1234;
    uint8_t buf[beacon_wire::SIZE];
    beacon_wire::encode(b, buf);
    CHECK_EQ(beacon_wire::SIZE, 20);
    CHECK_EQ(memcmp(buf, "POMO", 4), 0);
    CHECK_EQ(buf[4], 1);
    CHECK_EQ(buf[7], 0x34);
    CHECK_EQ(buf[8], 0x12);

    Beacon d = {};
    CHECK_EQ(beacon_wire::decode(buf, sizeof(buf), &d), true);
    CHECK_STR(d.name, "Ava");
    CHECK_EQ(d.in_session, true);
    CHECK_EQ(d.focused, false);
    CHECK_EQ(d.pct, 87);
    CHECK_EQ(d.seq, 0x1234);

    // Not ours: short, wrong magic, newer version, bad %
    CHECK_EQ(beacon_wire::decode(buf, sizeof(buf) - 1, &d), false);
    uint8_t bad[beacon_wire::SIZE];
    memcpy(bad, buf, sizeof(bad)); bad[0] = 'X';
    CHECK_EQ(beacon_wire::decode(bad, sizeof(bad), &d), false);
    memcpy(bad, buf, sizeof(bad)); bad[4] = 2;
    CHECK_EQ(beacon_wire::decode(bad, sizeof(bad), &d), false);
    memcpy(bad, buf, sizeof(bad)); bad[6] = 101;
    CHECK_EQ(beacon_wire::decode(bad, sizeof(bad), &d), false);
    CHECK_EQ(beacon_wire::decode(nullptr, 20, &d), false);

    // A longer packet (a later version adding fields at the end) still decodes
    uint8_t longer[32] = {};
    memcpy(longer, buf, sizeof(buf));
    CHECK_EQ(beacon_wire::decode(longer, sizeof(longer), &d), true);

    // A name with no terminator, or cut mid-character, comes out clean
    memcpy(bad, buf, sizeof(bad));
    memcpy(bad + 9, "ABCDEFGHIJK", 11);
    CHECK_EQ(beacon_wire::decode(bad, sizeof(bad), &d), true);
    CHECK_STR(d.name, "ABCDEFGHIJ");
    memcpy(bad + 9, "Zoe\xC3\xAB\xC3\xAB\xC3\xAB\xC3", 11);
    beacon_wire::decode(bad, sizeof(bad), &d);
    CHECK_STR(d.name, "Zoe\xC3\xAB\xC3\xAB\xC3\xAB");

    // The encoder cuts long names at a character boundary too
    Beacon longname = make("", false, true);
    strcpy(longname.name, "Zoeyy\xC3\xAB\xC3\xAB");  // 9 bytes, fits
    beacon_wire::encode(longname, buf);
    beacon_wire::decode(buf, sizeof(buf), &d);
    CHECK_STR(d.name, "Zoeyy\xC3\xAB\xC3\xAB");
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

static void test_summary()
{
    puts("what the session sees");
    CircleCore c;
    CHECK_EQ(c.friendsInSession(), 0);
    CHECK_EQ(c.friendDrifting(), false);
    c.onBeacon(AVA, make("Ava", true, true), 1000);
    c.onBeacon(DEV, make("Dev", false, true, 82), 1000);   // nearby but resting
    CHECK_EQ(c.count(), 2);                                // both set the multiplier
    CHECK_EQ(c.friendsInSession(), 1);
    CHECK_EQ(c.friendDrifting(), false);
    c.onBeacon(DEV, make("Dev", false, false, 82), 1500);  // not studying: no pause
    CHECK_EQ(c.friendDrifting(), false);
    c.onBeacon(AVA, make("Ava", true, false), 2000);
    CHECK_EQ(c.friendDrifting(), true);
    c.onBeacon(DEV, make("Dev", true, true), 2500);
    CHECK_EQ(c.friendsInSession(), 2);
}

int main()
{
    test_wire();
    test_join_and_change();
    test_timeout();
    test_full_circle();
    test_summary();
    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    puts("all passed");
    return 0;
}
