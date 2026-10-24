#include "session.hpp"

#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "session";

static const char *phase_name(Phase p)
{
    switch (p) {
        case Phase::STUDY: return "study";
        case Phase::BREAK: return "break";
        case Phase::LONG_REST: return "long rest";
        case Phase::READY: return "ready";
        default: return "idle";
    }
}

Session &Session::instance()
{
    static Session inst;
    return inst;
}

void Session::init()
{
    mutex = xSemaphoreCreateMutex();
    snapshot_queue = xQueueCreate(1, sizeof(SessionSnapshot));
    event_queues[event_queue_count++] = xQueueCreate(8, sizeof(SessionEvent));

    esp_timer_create_args_t args = {};
    args.callback = onTick;
    args.name = "session";
    esp_timer_handle_t timer;
    ESP_ERROR_CHECK(esp_timer_create(&args, &timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, 1000 * 1000));

    xSemaphoreTake(mutex, portMAX_DELAY);
    publish(nullptr);
    xSemaphoreGive(mutex);
}

void Session::publish(const SessionEvent *event)
{
    SessionSnapshot snap;
    core.snapshot(&snap);
    xQueueOverwrite(snapshot_queue, &snap);
    if (!event) return;
    for (int i = 0; i < event_queue_count; i++) {
        if (xQueueSend(event_queues[i], event, 0) != pdTRUE)
            ESP_LOGW(TAG, "Event queue %d full, dropped event %d", i, (int)event->type);
    }
}

QueueHandle_t Session::subscribe()
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    QueueHandle_t q = nullptr;
    if (event_queue_count < MAX_EVENT_QUEUES) {
        q = xQueueCreate(8, sizeof(SessionEvent));
        event_queues[event_queue_count++] = q;
    }
    xSemaphoreGive(mutex);
    return q;
}

void Session::onTick(void *arg)
{
    Session &s = instance();
    SessionEvent event;
    xSemaphoreTake(s.mutex, portMAX_DELAY);
    const Phase before = s.core.phase();
    const bool happened = s.core.tick(&event);
    s.publish(happened ? &event : nullptr);
    const Phase after = s.core.phase();
    xSemaphoreGive(s.mutex);

    if (happened) {
        ESP_LOGI(TAG, "%s -> %s", phase_name(before), phase_name(after));
        if (event.type == SessionEventType::STUDY_ENDED) {
            ESP_LOGI(TAG, "Session scored %d%% (%d/%d min), XP %+d, total %d, Lv %d",
                     event.result.pct, event.result.focused_min, event.result.elapsed_min,
                     event.result.xp_delta, event.result.xp, event.result.level);
        }
    }
}

bool Session::start(int study_min, int break_min, bool test)
{
    SessionEvent event = { SessionEventType::STARTED, {} };
    xSemaphoreTake(mutex, portMAX_DELAY);
    const bool ok = core.start(study_min, break_min, test);
    if (ok) publish(&event);
    xSemaphoreGive(mutex);
    if (ok) ESP_LOGI(TAG, "Started %d / %d%s", study_min, break_min, test ? " with test timing (30 s phases)" : "");
    return ok;
}

void Session::endEarly()
{
    SessionEvent event;
    xSemaphoreTake(mutex, portMAX_DELAY);
    const bool ok = core.endEarly(&event);
    if (ok) publish(&event);
    xSemaphoreGive(mutex);
    if (ok) {
        ESP_LOGI(TAG, "Ended early at %d%% (%d/%d min), XP %+d", event.result.pct,
                 event.result.focused_min, event.result.elapsed_min, event.result.xp_delta);
    }
}

void Session::endBreak()
{
    SessionEvent event;
    xSemaphoreTake(mutex, portMAX_DELAY);
    const Phase before = core.phase();
    const bool ok = core.endBreak(&event);
    if (ok) publish(&event);
    xSemaphoreGive(mutex);
    if (ok) ESP_LOGI(TAG, "Ended %s", phase_name(before));
}

void Session::endSet()
{
    SessionEvent event;
    xSemaphoreTake(mutex, portMAX_DELAY);
    const Phase before = core.phase();
    const bool ok = core.endSet(&event);
    if (ok) publish(&event);
    xSemaphoreGive(mutex);
    if (ok) ESP_LOGI(TAG, "Ended the set during %s", phase_name(before));
}

void Session::setDrift(bool active, const char *label, DriftKind kind)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    core.setDrift(active, label, kind);
    publish(nullptr);
    xSemaphoreGive(mutex);
}

void Session::setFocus(CameraState state, const char *label)
{
    // No publish: a tick only changes the drift on the next second, in onTick
    xSemaphoreTake(mutex, portMAX_DELAY);
    core.setFocus(state, label);
    xSemaphoreGive(mutex);
}

void Session::setCircle(int friends_in_session, bool friend_drifting)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    core.setCircle(friends_in_session, friend_drifting);
    publish(nullptr);
    xSemaphoreGive(mutex);
}

void Session::setXp(int xp)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    core.setXp(xp);
    publish(nullptr);
    xSemaphoreGive(mutex);
}

SessionSnapshot Session::snapshot()
{
    SessionSnapshot snap;
    xSemaphoreTake(mutex, portMAX_DELAY);
    core.snapshot(&snap);
    xSemaphoreGive(mutex);
    return snap;
}
