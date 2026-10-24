#include "health.hpp"

#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "health";

static const TickType_t STALL_TICKS = pdMS_TO_TICKS(5000);
static const TickType_t HEAP_LOG_TICKS = pdMS_TO_TICKS(60 * 1000);
// esp_lvgl_port's task
static const char *const UI_TASK = "taskLVGL";

static volatile TickType_t s_ui_alive = 0;

void health_log_heap(const char *stage)
{
    ESP_LOGI(TAG, "[heap] %s: free %u, largest block %u, min ever %u", stage,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT));
}

void health_ui_alive()
{
    s_ui_alive = xTaskGetTickCount();
}

static const char *state_name(eTaskState state)
{
    switch (state) {
        case eRunning: return "running";
        case eReady: return "ready (starved?)";
        case eBlocked: return "blocked (waiting on a lock or queue?)";
        case eSuspended: return "suspended";
        case eDeleted: return "deleted";
        default: return "?";
    }
}

static void health_task(void *)
{
    TickType_t last_heap_log = xTaskGetTickCount();
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        const TickType_t now = xTaskGetTickCount();

        if (now - last_heap_log >= HEAP_LOG_TICKS) {
            last_heap_log = now;
            health_log_heap("every minute");
        }

        const TickType_t quiet = now - s_ui_alive;
        if (quiet < STALL_TICKS) continue;

        TaskHandle_t ui = xTaskGetHandle(UI_TASK);
        ESP_LOGE(TAG, "[ui] stalled: the UI task hasn't run for %u ms", (unsigned)pdTICKS_TO_MS(quiet));
        if (ui) {
            ESP_LOGE(TAG, "[ui] %s is %s, %u bytes of stack never used", UI_TASK, state_name(eTaskGetState(ui)),
                     (unsigned)(uxTaskGetStackHighWaterMark(ui) * sizeof(StackType_t)));
        }
        health_log_heap("at the stall");
        ESP_LOGE(TAG, "[ui] restarting. Please send this log, including the core dump below.");
        vTaskDelay(pdMS_TO_TICKS(200)); // let the log drain
        abort();                        // the core dump shows where every task was
    }
}

void health_start()
{
    s_ui_alive = xTaskGetTickCount();
    // Above the UI task's priority, so it still runs if the UI task spins
    xTaskCreate(health_task, "health", 3072, nullptr, 10, nullptr);
}
