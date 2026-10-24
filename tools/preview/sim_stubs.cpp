// Host implementations of the ESP-IDF, FreeRTOS and HAL pieces the UI code touches.
// Single-threaded: the preview drives timers and LVGL from one loop.

#include <deque>
#include <vector>
#include <string.h>

#include "esp_timer.h"
#include "freertos/queue.h"
#include "hal/encoder.h"
#include "system/system.hpp"

// ---- Queues ----

struct sim_queue {
    size_t length;
    size_t item_size;
    std::deque<std::vector<uint8_t>> items;
};

QueueHandle_t xQueueCreate(size_t length, size_t item_size)
{
    return new sim_queue{length, item_size, {}};
}

BaseType_t xQueueSend(QueueHandle_t q, const void *item, TickType_t)
{
    if (q->items.size() >= q->length) return pdFALSE;
    const uint8_t *p = (const uint8_t *)item;
    q->items.emplace_back(p, p + q->item_size);
    return pdTRUE;
}

BaseType_t xQueueOverwrite(QueueHandle_t q, const void *item)
{
    q->items.clear();
    return xQueueSend(q, item, 0);
}

BaseType_t xQueueReceive(QueueHandle_t q, void *item, TickType_t)
{
    if (q->items.empty()) return pdFALSE;
    memcpy(item, q->items.front().data(), q->item_size);
    q->items.pop_front();
    return pdTRUE;
}

// ---- esp_timer ----

struct sim_timer {
    esp_timer_cb_t cb;
    void *arg;
    uint64_t period_us;
    uint64_t next_us;
};

static std::vector<sim_timer *> s_timers;
static uint64_t s_now_us = 0;

esp_err_t esp_timer_create(const esp_timer_create_args_t *args, esp_timer_handle_t *out)
{
    *out = new sim_timer{args->callback, args->arg, 0, 0};
    s_timers.push_back(*out);
    return ESP_OK;
}

esp_err_t esp_timer_start_periodic(esp_timer_handle_t t, uint64_t period_us)
{
    t->period_us = period_us;
    t->next_us = s_now_us + period_us;
    return ESP_OK;
}

int64_t esp_timer_get_time(void) { return (int64_t)s_now_us; }

void sim_timers_advance(uint64_t us)
{
    const uint64_t end = s_now_us + us;
    for (;;) {
        sim_timer *due = nullptr;
        for (sim_timer *t : s_timers)
            if (t->period_us && t->next_us <= end && (!due || t->next_us < due->next_us)) due = t;
        if (!due) break;
        s_now_us = due->next_us;
        due->next_us += due->period_us;
        due->cb(due->arg);
    }
    s_now_us = end;
}

// ---- Encoder (no hardware: the preview calls the hold callback itself) ----

static encoder_hold_cb_t s_hold_cb;

extern "C" esp_err_t encoder_init(void) { return ESP_OK; }
extern "C" lv_indev_t *encoder_get_indev(void) { return nullptr; }
extern "C" void encoder_set_hold_cb(encoder_hold_cb_t cb) { s_hold_cb = cb; }

void sim_encoder_hold(void)
{
    if (s_hold_cb) s_hold_cb();
}

// ---- System clock (fixed, settable) ----

long g_sim_epoch = 0;

SystemAPI &SystemAPI::instance()
{
    static SystemAPI inst;
    return inst;
}

long SystemAPI::getCurrentTime() const { return g_sim_epoch; }

int SystemAPI::setCurrentTime(long epochSeconds) const
{
    g_sim_epoch = epochSeconds;
    return 0;
}
