#include "circle.hpp"

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <app/circle_core.hpp>
#include <app/session.hpp>
#include <ui/app_state.hpp>
#include <utilities.h>

static const char *TAG = "circle";

static const uint32_t TICK_MS = 500;

// Every Pomo listens and broadcasts on this Wi-Fi channel. No router: the station never connects.
static const uint8_t CHANNEL = 1;
static const uint8_t BROADCAST[ESP_NOW_ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

struct Packet {
    uint8_t mac[6];
    uint8_t len;
    uint8_t data[circle_wire::MAX_SIZE];
};

struct Command {
    enum Type : uint8_t { INVITE, START, CANCEL, ACCEPT, DECLINE } type;
    uint8_t mac[6];
    uint8_t study, brk;
    bool test;
};

static QueueHandle_t s_rx;
static QueueHandle_t s_commands;
static bool s_radio;
static uint8_t s_mac[6];
static int s_send_failures;
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
    MsgType type;
    if (!s_rx || !circle_wire::type(data, len, &type)) return;
    Packet p;
    memcpy(p.mac, mac, 6);
    p.len = (uint8_t)(len < sizeof(p.data) ? len : sizeof(p.data));
    memcpy(p.data, data, p.len);
    xQueueSend(s_rx, &p, 0); // full: drop it; beacons repeat, invites and declines are resent
}

static void post(const Command &cmd)
{
    if (s_commands && xQueueSend(s_commands, &cmd, 0) != pdTRUE) ESP_LOGW(TAG, "Command queue full");
}

void circle_invite(const uint8_t mac[6], int study, int brk, bool test)
{
    Command cmd = {};
    cmd.type = Command::INVITE;
    memcpy(cmd.mac, mac, 6);
    cmd.study = (uint8_t)study;
    cmd.brk = (uint8_t)brk;
    cmd.test = test;
    post(cmd);
}

void circle_start() { post(Command{Command::START, {}, 0, 0, false}); }
void circle_cancel() { post(Command{Command::CANCEL, {}, 0, 0, false}); }
void circle_answer(bool accept) { post(Command{accept ? Command::ACCEPT : Command::DECLINE, {}, 0, 0, false}); }

static uint32_t now_ms() { return (uint32_t)(esp_timer_get_time() / 1000); }
static bool reached(uint32_t now, uint32_t deadline) { return (int32_t)(now - deadline) >= 0; }

#if POMO_FAKE_PEER
static void fake_heard(const uint8_t *data, size_t len);
#endif

static void broadcast(const uint8_t *data, size_t len)
{
    if (s_radio) {
        // Fails only when the Wi-Fi TX queue is full; beacons repeat and invites are resent
        const esp_err_t err = esp_now_send(BROADCAST, data, len);
        if (err != ESP_OK && s_send_failures++ % 20 == 0)
            ESP_LOGW(TAG, "Not sent (%d so far): %s", s_send_failures, esp_err_to_name(err));
    }
#if POMO_FAKE_PEER
    fake_heard(data, len);
#endif
}

// What this device tells the circle about itself
static Beacon own_beacon(const SessionSnapshot &snap, const CircleCore &core, int last_pct, uint16_t seq)
{
    Beacon b = {};
    portENTER_CRITICAL(&s_name_lock);
    memcpy(b.name, s_name, sizeof(b.name));
    portEXIT_CRITICAL(&s_name_lock);
    b.phase = snap.phase;
    b.index = (uint8_t)snap.session_index;
    b.focused = !snap.drifting;
    b.pct = (uint8_t)last_pct;
    b.seq = seq;
    b.group = core.group();
    return b;
}

static const char *role_name(GroupRole role)
{
    switch (role) {
        case GroupRole::HOSTING: return "hosting";
        case GroupRole::INVITED: return "invited";
        case GroupRole::JOINED: return "joined";
        case GroupRole::IN_SET: return "in a group";
        default: return "solo";
    }
}

// Pass a changed circle to the session rules and the UI
static void publish(const CircleCore &core)
{
    Session::instance().setCircle(core.membersStudying(), core.memberDrifting());

    AppState::Friend list[AppState::MAX_FRIENDS] = {};
    const int count = core.count() < AppState::MAX_FRIENDS ? core.count() : AppState::MAX_FRIENDS;
    for (int i = 0; i < count; i++) {
        const CirclePeer &p = core.peer(i);
        const Beacon &b = p.beacon;
        strlcpy(list[i].name, b.name, sizeof(list[i].name));
        // A friend who isn't studying can't be drifting, and their % is the last session's
        list[i].studying = b.studying();
        list[i].focused = !b.studying() || b.focused;
        list[i].pct = b.studying() ? 0 : b.pct;
        memcpy(list[i].mac, p.mac, 6);
        list[i].status = core.status(i);
    }

    AppState::CircleView view = {};
    view.role = core.role();
    strlcpy(view.host, core.hostName(), sizeof(view.host));
    view.study = core.invitation().study;
    view.brk = core.invitation().brk;
    view.test = core.invitation().test;
    view.can_invite = core.canInvite();
    view.can_start = core.canStart();
    AppState::instance().postFriends(list, count, view);

    ESP_LOGI(TAG, "%d friend(s), %s, %d studying with us%s", core.count(), role_name(core.role()),
             core.membersStudying(), core.memberDrifting() ? ", one drifting" : "");
}

static void handle_packet(CircleCore &core, const Packet &p, uint32_t now)
{
    MsgType type;
    if (!circle_wire::type(p.data, p.len, &type)) return;
    switch (type) {
        case MsgType::BEACON: {
            Beacon b;
            if (circle_wire::decode(p.data, p.len, &b)) core.onBeacon(p.mac, b, now);
            break;
        }
        case MsgType::INVITE: {
            Invite m;
            if (circle_wire::decode(p.data, p.len, &m)) core.onInvite(p.mac, m, now);
            break;
        }
        case MsgType::DECLINE: {
            Decline m;
            if (circle_wire::decode(p.data, p.len, &m)) core.onDecline(p.mac, m, now);
            break;
        }
    }
}

static void handle_command(CircleCore &core, const Command &cmd, uint32_t now)
{
    bool ok = false;
    switch (cmd.type) {
        case Command::INVITE: {
            uint32_t group = 0;
            while (group == 0) group = esp_random();
            ok = core.invite(cmd.mac, cmd.study, cmd.brk, cmd.test, group, now);
            break;
        }
        case Command::START: ok = core.startHosted(); break;
        case Command::CANCEL: ok = core.cancelHosted(); break;
        case Command::ACCEPT: ok = core.accept(); break;
        case Command::DECLINE: ok = core.decline(); break;
    }
    if (!ok) ESP_LOGW(TAG, "Command %d doesn't apply now (%s)", (int)cmd.type, role_name(core.role()));
}

#if POMO_FAKE_PEER
// ---- Scripted friends for one board, fed through circle_on_receive like real packets ----

static const uint8_t FAKE_AVA[6] = {0x02, 'F', 'A', 'K', 'E', 1};
static const uint8_t FAKE_DEV[6] = {0x02, 'F', 'A', 'K', 'E', 2};

static struct {
    uint32_t ava_group;         // invited to (or in) this group
    uint32_t ava_join_ms;       // when she shows it in her beacon
    uint32_t dev_no_group;      // an invite Dev will decline...
    uint32_t dev_no_ms;         // ...at this time
    uint32_t dev_group;         // Dev's own group, inviting this device
    bool dev_accepted;
    uint32_t dev_until_ms;      // his invite runs out
    uint32_t dev_next_ms;       // his next invite
} s_fake = {0, 0, 0, 0, 0, false, 0, 20000};

static void fake_send(const uint8_t from[6], const uint8_t *data, size_t len) { circle_on_receive(from, data, len); }

static void dev_stop(uint32_t now)
{
    s_fake.dev_group = 0;
    s_fake.dev_accepted = false;
    s_fake.dev_next_ms = now + 180000;
}

// The fake friends hear what this device sends
static void fake_heard(const uint8_t *data, size_t len)
{
    const uint32_t now = now_ms();
    Invite inv;
    if (circle_wire::decode(data, len, &inv)) {
        if (memcmp(inv.to, FAKE_AVA, 6) == 0 && s_fake.ava_group == 0) {
            s_fake.ava_group = inv.group;
            s_fake.ava_join_ms = now + 2000;
        } else if (memcmp(inv.to, FAKE_DEV, 6) == 0 && s_fake.dev_no_group != inv.group) {
            s_fake.dev_no_group = inv.group;
            s_fake.dev_no_ms = now + 2000;
        }
    }
    Decline no;
    if (circle_wire::decode(data, len, &no) && memcmp(no.to, FAKE_DEV, 6) == 0 && no.group == s_fake.dev_group)
        dev_stop(now);
}

// Once per beacon interval. `own` is this device's beacon.
static void fake_peers(uint32_t now, uint32_t since_boot_ms, const Beacon &own, const CircleCore &core)
{
    static uint16_t seq = 0;
    uint8_t packet[circle_wire::MAX_SIZE];
    size_t len;

    // Ava: in step with this device once she has joined; she leaves when its group goes
    Beacon ava = {};
    strlcpy(ava.name, "Ava", sizeof(ava.name));
    ava.phase = Phase::IDLE;
    ava.index = 1;
    ava.pct = 90;
    ava.seq = seq;
    if (s_fake.ava_group && reached(now, s_fake.ava_join_ms)) {
        if (own.group != s_fake.ava_group) {
            s_fake.ava_group = 0;
        } else {
            ava.group = s_fake.ava_group;
            if (own.inSet()) {
                ava.phase = own.phase;
                ava.index = own.index;
            }
        }
    }
    const uint32_t t = (since_boot_ms / 1000) % 120;
    ava.focused = !(ava.studying() && t >= 40 && t < 55);
    len = circle_wire::encode(ava, packet);
    fake_send(FAKE_AVA, packet, len);

    // Dev: invites this device now and then; once it joins he starts, then keeps in step
    if (!s_fake.dev_group && reached(now, s_fake.dev_next_ms) && core.role() == GroupRole::NONE &&
        own.phase == Phase::IDLE) {
        s_fake.dev_group = esp_random() | 1;
        s_fake.dev_until_ms = now + CircleCore::INVITE_TIMEOUT_MS;
    }
    if (s_fake.dev_group && !s_fake.dev_accepted) {
        if (own.group == s_fake.dev_group) s_fake.dev_accepted = true;
        else if (reached(now, s_fake.dev_until_ms)) dev_stop(now);
    } else if (s_fake.dev_group && own.group != s_fake.dev_group) {
        dev_stop(now); // this device left
    }
    Beacon dev = {};
    strlcpy(dev.name, "Dev", sizeof(dev.name));
    dev.phase = Phase::IDLE;
    dev.index = 1;
    dev.focused = true;
    dev.pct = 82;
    dev.seq = seq;
    dev.group = s_fake.dev_group;
    if (s_fake.dev_accepted) {
        dev.phase = own.inSet() ? own.phase : Phase::STUDY;
        dev.index = own.inSet() ? own.index : 1;
    }
    len = circle_wire::encode(dev, packet);
    fake_send(FAKE_DEV, packet, len);

    if (s_fake.dev_group && !s_fake.dev_accepted) {
        Invite inv = {};
        memcpy(inv.to, s_mac, 6);
        inv.group = s_fake.dev_group;
        inv.study = 25;
        inv.brk = 5;
        inv.test = true;
        len = circle_wire::encode(inv, packet);
        fake_send(FAKE_DEV, packet, len);
    }
    if (s_fake.dev_no_group && reached(now, s_fake.dev_no_ms)) {
        Decline no = {};
        memcpy(no.to, s_mac, 6);
        no.group = s_fake.dev_no_group;
        len = circle_wire::encode(no, packet);
        fake_send(FAKE_DEV, packet, len);
        s_fake.dev_no_group = 0;
    }
    seq++;
}
#endif

// ESP-NOW calls this on the Wi-Fi task
static void on_espnow_receive(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (info && data && len > 0) circle_on_receive(info->src_addr, data, (size_t)len);
}

// Wi-Fi as a station that never connects (no esp_netif: nothing here uses IP), then ESP-NOW
// with a broadcast peer. BT coexistence is on in sdkconfig.defaults.
static bool radio_init()
{
    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Event loop: %s", esp_err_to_name(err));
        return false;
    }
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, BROADCAST, sizeof(BROADCAST));
    peer.channel = CHANNEL;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;

    if ((err = esp_wifi_init(&cfg)) != ESP_OK ||
        (err = esp_wifi_set_storage(WIFI_STORAGE_RAM)) != ESP_OK ||
        (err = esp_wifi_set_mode(WIFI_MODE_STA)) != ESP_OK ||
        (err = esp_wifi_start()) != ESP_OK ||
        (err = esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE)) != ESP_OK ||
        (err = esp_now_init()) != ESP_OK ||
        (err = esp_now_register_recv_cb(on_espnow_receive)) != ESP_OK ||
        (err = esp_now_add_peer(&peer)) != ESP_OK) {
        ESP_LOGE(TAG, "ESP-NOW failed to start: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

static void circle_task(void *arg)
{
    static CircleCore core;
    core.setMac(s_mac);
    QueueHandle_t events = Session::instance().subscribe();
    if (!events) ESP_LOGW(TAG, "No session event queue left: the last session's %% stays 0");

    int last_pct = 0;
    uint16_t seq = 0;
    const uint32_t start = now_ms();
    uint32_t next_beacon = start;
    Beacon last_sent = {};
    TickType_t wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(TICK_MS));
        const uint32_t now = now_ms();

        SessionEvent event;
        while (events && xQueueReceive(events, &event, 0) == pdTRUE) {
            if (event.type == SessionEventType::STUDY_ENDED || event.type == SessionEventType::ENDED_EARLY)
                last_pct = event.result.pct;
        }

        SessionSnapshot snap = Session::instance().snapshot();
        core.setSelf(snap.phase, snap.session_index);

        Packet p;
        while (xQueueReceive(s_rx, &p, 0) == pdTRUE) handle_packet(core, p, now);
        Command cmd;
        while (xQueueReceive(s_commands, &cmd, 0) == pdTRUE) handle_command(core, cmd, now);
        core.tick(now);

        // The group starting or moving on to the next session
        const GroupAction action = core.takeAction();
        if (action.type == GroupAction::START) {
            ESP_LOGI(TAG, "Group start: %d / %d%s", action.study, action.brk, action.test ? ", test timing" : "");
            Session::instance().start(action.study, action.brk, action.test);
        } else if (action.type == GroupAction::NEXT) {
            ESP_LOGI(TAG, "Group moved on: next session");
            Session::instance().endBreak();
        }
        if (action.type != GroupAction::NONE) {
            snap = Session::instance().snapshot();
            core.setSelf(snap.phase, snap.session_index);
        }

        uint8_t packet[circle_wire::MAX_SIZE];
        size_t len;
        while (core.nextMessage(now, packet, &len)) broadcast(packet, len);

        // Beacons every 1.5 s, and straight away when this device's state changes, so the
        // group follows quickly
        Beacon own = own_beacon(snap, core, last_pct, seq);
        const bool news = own.phase != last_sent.phase || own.index != last_sent.index ||
                          own.group != last_sent.group || own.focused != last_sent.focused;
        if (news || reached(now, next_beacon)) {
            next_beacon = now + CircleCore::BEACON_INTERVAL_MS;
            seq++;
            len = circle_wire::encode(own, packet);
            broadcast(packet, len);
            last_sent = own;
#if POMO_FAKE_PEER
            fake_peers(now, now - start, own, core);
#endif
        }

        if (core.takeChanged()) publish(core);
    }
}

void circle_init()
{
    s_rx = xQueueCreate(12, sizeof(Packet));
    s_commands = xQueueCreate(4, sizeof(Command));
    // Without the radio the circle still runs (empty, or with fake friends)
    s_radio = radio_init();
    if (esp_read_mac(s_mac, ESP_MAC_WIFI_STA) != ESP_OK) ESP_LOGW(TAG, "No Wi-Fi MAC: invites can't reach us");
    xTaskCreate(circle_task, "circle", 4096, nullptr, 3, nullptr);
    ESP_LOGI(TAG, "Study circle ready: %s, channel %u, " MACSTR "%s", s_radio ? "ESP-NOW" : "no radio",
             (unsigned)CHANNEL, MAC2STR(s_mac), POMO_FAKE_PEER ? ", fake friends Ava and Dev" : "");
}
