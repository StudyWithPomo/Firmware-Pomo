#pragma once
// Thread-safe owner of the study session. A 1 s esp_timer drives SessionCore, so the timer
// stays accurate however busy the UI is. Commands may come from any task (UI, laptop link,
// study circle). State goes out through two queues that only the UI drains:
//   snapshots(): latest SessionSnapshot, length 1, overwritten on every change
//   events():    SessionEvent, in order, so nothing like "study ended" is missed
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
    void setDrift(bool active, const char *label);
    void setCircle(int friends_in_session, bool friend_drifting);
    void setXp(int xp);

    // Copy of the current state (any task)
    SessionSnapshot snapshot();

    QueueHandle_t snapshots() const { return snapshot_queue; }
    QueueHandle_t events() const { return event_queue; }

private:
    Session() = default;
    static void onTick(void *arg);
    // Call with the mutex held
    void publish(const SessionEvent *event);

    SessionCore core;
    SemaphoreHandle_t mutex = nullptr;
    QueueHandle_t snapshot_queue = nullptr;
    QueueHandle_t event_queue = nullptr;
};
