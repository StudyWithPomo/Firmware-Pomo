#pragma once
// Thread-safe owner of the study session. A 1 s esp_timer drives SessionCore, so the timer
// stays accurate however busy the UI is. Commands may come from any task (UI, laptop link,
// study circle). State goes out through two queues that only the UI drains:
//   snapshots(): latest SessionSnapshot, length 1, overwritten on every change
//   events():    SessionEvent, in order, so nothing like "study ended" is missed
// Other tasks (the laptop link) get their own copy of the events through subscribe().
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "session_core.hpp"

class Session {
public:
    static Session &instance();

    void init();

    bool start(int study_min, int break_min, bool test = false);
    void endEarly();
    void endBreak();    // break -> next study session, long rest -> idle
    void endSet();      // whole set -> idle (scored first if studying)
    void setDrift(bool active, const char *label, DriftKind kind = DriftKind::SITE);
    void setFocus(CameraState state, const char *label);
    void setCircle(int friends, bool friend_drifting);
    void setXp(int xp);

    // The laptop's heartbeat. The dashboard sends a ping every 2 s while it's coupled; any
    // line from it counts as heard. Once a ping has come in, LAPTOP_TIMEOUT_S of silence
    // drops the laptop's drift and pauses a study session (snapshot.paused) until the laptop
    // is heard again or forgetLaptop() (the device's "Go on alone"). A dashboard that never
    // pings never pauses anything.
    static constexpr int LAPTOP_TIMEOUT_S = 6;
    void laptopHeard(bool ping);
    void forgetLaptop();

    // Copy of the current state (any task)
    SessionSnapshot snapshot();

    QueueHandle_t snapshots() const { return snapshot_queue; }
    QueueHandle_t events() const { return event_queues[0]; }
    // A new event queue that receives every event from now on (call after init)
    QueueHandle_t subscribe();

private:
    Session() = default;
    static void onTick(void *arg);
    // Call with the mutex held
    void publish(const SessionEvent *event);
    bool laptopLinked() const;

    bool laptop_armed = false;      // a ping came in since boot or forgetLaptop()
    int64_t laptop_heard_us = 0;
    bool laptop_was_linked = false;

    SessionCore core;
    SemaphoreHandle_t mutex = nullptr;
    QueueHandle_t snapshot_queue = nullptr;
    static const int MAX_EVENT_QUEUES = 3;
    QueueHandle_t event_queues[MAX_EVENT_QUEUES] = {};
    int event_queue_count = 0;
};
