#include "circle_core.hpp"

#include <string.h>

#include "utilities.h"

static const uint8_t MAGIC[4] = {'P', 'O', 'M', 'O'};

void beacon_wire::encode(const Beacon &b, uint8_t *out)
{
    memset(out, 0, SIZE);
    memcpy(out, MAGIC, 4);
    out[4] = VERSION;
    out[5] = (b.in_session ? FLAG_IN_SESSION : 0) | (b.focused ? FLAG_FOCUSED : 0);
    out[6] = b.pct > 100 ? 100 : b.pct;
    out[7] = (uint8_t)(b.seq & 0xFF);
    out[8] = (uint8_t)(b.seq >> 8);
    copy_utf8((char *)out + 9, 11, b.name);
}

bool beacon_wire::decode(const uint8_t *data, size_t len, Beacon *out)
{
    if (!data || len < SIZE || memcmp(data, MAGIC, 4) != 0 || data[4] != VERSION) return false;
    if (data[6] > 100) return false;
    out->in_session = (data[5] & FLAG_IN_SESSION) != 0;
    out->focused = (data[5] & FLAG_FOCUSED) != 0;
    out->pct = data[6];
    out->seq = (uint16_t)(data[7] | (data[8] << 8));
    // The name field may be missing its terminator, or be cut mid-character by a bad sender
    char name[12];
    memcpy(name, data + 9, 11);
    name[11] = '\0';
    copy_utf8(out->name, sizeof(out->name), name);
    return true;
}

bool CircleCore::onBeacon(const uint8_t mac[6], const Beacon &b, uint32_t now_ms)
{
    for (int i = 0; i < count_; i++) {
        CirclePeer &p = peers_[i];
        if (memcmp(p.mac, mac, 6) != 0) continue;
        const bool changed = strcmp(p.beacon.name, b.name) != 0 || p.beacon.in_session != b.in_session ||
                             p.beacon.focused != b.focused || p.beacon.pct != b.pct;
        p.beacon = b;
        p.last_heard_ms = now_ms;
        return changed;
    }
    if (count_ >= MAX_PEERS) return false;
    CirclePeer &p = peers_[count_++];
    memcpy(p.mac, mac, 6);
    p.beacon = b;
    p.last_heard_ms = now_ms;
    return true;
}

bool CircleCore::expire(uint32_t now_ms)
{
    bool dropped = false;
    int kept = 0;
    for (int i = 0; i < count_; i++) {
        // Unsigned difference, so it still works when the ms clock wraps
        if (now_ms - peers_[i].last_heard_ms > PEER_TIMEOUT_MS) {
            dropped = true;
            continue;
        }
        if (kept != i) peers_[kept] = peers_[i];
        kept++;
    }
    count_ = kept;
    return dropped;
}

int CircleCore::friendsInSession() const
{
    int n = 0;
    for (int i = 0; i < count_; i++)
        if (peers_[i].beacon.in_session) n++;
    return n;
}

bool CircleCore::friendDrifting() const
{
    for (int i = 0; i < count_; i++)
        if (peers_[i].beacon.in_session && !peers_[i].beacon.focused) return true;
    return false;
}
