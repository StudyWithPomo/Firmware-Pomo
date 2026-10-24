#include "circle.hpp"

#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <app/circle_core.hpp>
#include <app/session.hpp>
#include <ui/app_state.hpp>
#include <utilities.h>

static const char *TAG = "circle";

static const uint32_t TICK_MS = 500;

struct Received {
    uint8_t mac[6];
    Beacon beacon;
};

static QueueHandle_t s_rx;
static portMUX_TYPE s_name_lock = portMUX_INITIALIZER_UNLOCKED;
static char s_name[sizeof(Beacon::name)] = "";

void circle_set_name(const char *name)
{
    char copy[sizeof(s_name)];
    copy_utf8(copy, sizeof(copy), name);
    portENTER_CRITICAL(&s_name_lock);
    memcpy(s_name, copy, sizeof(s_name));
    portEXIT_CRITICAL(&s_name_lock);
}

void circle_on_receive(const uint8_t mac[6], const uint8_t *data, size_t len)
{
    Received r;
    if (!s_rx || !beacon_wire::decode(data, len, &r.beacon)) return;
    memcpy(r.mac, mac, 6);
    xQueueSend(s_rx, &r, 0); // full: drop it, the next one comes 1.5 s later
}

static uint32_t now_ms() { return (uint32_t)(esp_timer_get_time() / 1000); }

// What this device tells the circle about itself
static Beacon own_beacon(int last_pct, uint16_t seq)
{
    const SessionSnapshot snap = Session::instance().snapshot();
    Beacon b = {};
    portENTER_CRITICAL(&s_name_lock);
    memcpy(b.name, s_name, sizeof(b.name));
    portEXIT_CRITICAL(&s_name_lock);
    b.in_session = snap.phase == Phase::STUDY;
    b.focused = !snap.drifting;
    b.pct = (uint8_t)last_pct;
    b.seq = seq;
    return b;
}

// Pass a changed circle to the session rules and the UI
static void publish(const CircleCore &core)
{
    Session::instance().setCircle(core.count(), core.friendDrifting());

    AppState::Friend list[AppState::MAX_FRIENDS] = {};
    const int count = core.count() < AppState::MAX_FRIENDS ? core.count() : AppState::MAX_FRIENDS;
    for (int i = 0; i < count; i++) {
        const Beacon &b = core.peer(i).beacon;
        strlcpy(list[i].name, b.name, sizeof(list[i].name));
        // A friend who isn't studying can't be drifting, and their % is the last session's
        list[i].focused = !b.in_session || b.focused;
        list[i].pct = b.in_session ? 0 : b.pct;
    }
    AppState::instance().postFriends(list, count);
    ESP_LOGI(TAG, "%d friend(s), %d studying%s", core.count(), core.friendsInSession(),
             core.friendDrifting() ? ", one drifting" : "");
}

#if POMO_FAKE_PEER
// Scripted friends, sent through circle_on_receive like real packets. `t_ms` counts from start.
static void fake_peers(uint32_t t_ms)
{
    static const uint8_t AVA[6] = {0x02, 'F', 'A', 'K', 'E', 1};
    static const uint8_t DEV[6] = {0x02, 'F', 'A', 'K', 'E', 2};
    static uint16_t seq = 0;
    const uint32_t t = (t_ms / 1000) % 120;
    uint8_t packet[beacon_wire::SIZE];

    if (t < 100) {
        Beacon ava = {};
        strlcpy(ava.name, "Ava", sizeof(ava.name));
        ava.in_session = true;
        ava.focused = !(t >= 40 && t < 55);
        ava.seq = seq;
        beacon_wire::encode(ava, packet);
        circle_on_receive(AVA, packet, sizeof(packet));
    }

    Beacon dev = {};
    strlcpy(dev.name, "Dev", sizeof(dev.name));
    dev.focused = true;
    dev.pct = 82;
    dev.seq = seq;
    beacon_wire::encode(dev, packet);
    circle_on_receive(DEV, packet, sizeof(packet));
    seq++;
}
#endif

static void circle_task(void *arg)
{
    static CircleCore core;
    QueueHandle_t events = Session::instance().subscribe();
    if (!events) ESP_LOGW(TAG, "No session event queue left: the last session's %% stays 0");

    int last_pct = 0;
    uint16_t seq = 0;
    const uint32_t start = now_ms();
    uint32_t next_beacon = start;
    TickType_t wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(TICK_MS));
        const uint32_t now = now_ms();

        SessionEvent event;
        while (events && xQueueReceive(events, &event, 0) == pdTRUE) {
            if (event.type == SessionEventType::STUDY_ENDED || event.type == SessionEventType::ENDED_EARLY)
                last_pct = event.result.pct;
        }

        if ((int32_t)(now - next_beacon) >= 0) {
            next_beacon = now + CircleCore::BEACON_INTERVAL_MS;
            uint8_t packet[beacon_wire::SIZE];
            const Beacon own = own_beacon(last_pct, seq++);
            beacon_wire::encode(own, packet);
            // L2 broadcasts `packet` over ESP-NOW here
            ESP_LOGD(TAG, "beacon %u: %s%s", (unsigned)own.seq, own.in_session ? "studying" : "resting",
                     own.focused ? "" : ", drifting");
#if POMO_FAKE_PEER
            fake_peers(now - start);
#endif
        }

        bool changed = false;
        Received r;
        while (xQueueReceive(s_rx, &r, 0) == pdTRUE) changed |= core.onBeacon(r.mac, r.beacon, now);
        changed |= core.expire(now);
        if (changed) publish(core);
    }
}

void circle_init()
{
    s_rx = xQueueCreate(8, sizeof(Received));
    xTaskCreate(circle_task, "circle", 3072, nullptr, 3, nullptr);
    ESP_LOGI(TAG, "Study circle ready%s", POMO_FAKE_PEER ? " (fake friends Ava and Dev)" : "");
}
