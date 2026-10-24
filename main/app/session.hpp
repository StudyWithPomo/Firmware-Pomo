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

    bool start(int study_min, int break_min);
    void endEarly();
    void endBreak();    // break -> next study session, long rest -> idle
    void endSet();      // whole set -> idle (scored first if studying)
    void setDrift(bool active, const char *label, DriftKind kind = DriftKind::SITE);
    void setFocus(CameraState state, const char *label);
    void setCircle(int friends_in_session, bool friend_drifting);
    void setXp(int xp);

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

    SessionCore core;
    SemaphoreHandle_t mutex = nullptr;
    QueueHandle_t snapshot_queue = nullptr;
    static const int MAX_EVENT_QUEUES = 3;
    QueueHandle_t event_queues[MAX_EVENT_QUEUES] = {};
    int event_queue_count = 0;
};
