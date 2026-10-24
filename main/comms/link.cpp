#include "link.hpp"

#include <string.h>

#include "cJSON.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <app/session.hpp>
#include <ui/app_state.hpp>
#include <ui/pet/pet_data.h>

static const char *TAG = "link";

#define POMO_FW_VERSION "0.5"

static const uart_port_t LINK_UART = UART_NUM_0;  // the USB serial port, shared with logs
static const int UART_RX_BUF = 1024;

static SemaphoreHandle_t s_tx_mutex;
static QueueHandle_t s_events;

// ---- Sending ----

static void send_json(cJSON *obj)
{
    char *text = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);
    if (!text) return;
    link_send(text);
    cJSON_free(text);
}

void link_send(const char *json)
{
    if (!s_tx_mutex) return;
    xSemaphoreTake(s_tx_mutex, portMAX_DELAY);
    // Start on a fresh line in case a log line is half-written
    uart_write_bytes(LINK_UART, "\n", 1);
    uart_write_bytes(LINK_UART, json, strlen(json));
    uart_write_bytes(LINK_UART, "\n", 1);
    xSemaphoreGive(s_tx_mutex);
}

void link_send_knob(const char *ev)
{
    cJSON *o = cJSON_CreateObject();
    cJSON_AddStringToObject(o, "t", "knob");
    cJSON_AddStringToObject(o, "ev", ev);
    send_json(o);
}

void link_send_circle()
{
    int count = 0;
    const AppState::Friend *friends = AppState::instance().friends(&count);
    cJSON *o = cJSON_CreateObject();
    cJSON_AddStringToObject(o, "t", "circle");
    cJSON *list = cJSON_AddArrayToObject(o, "friends");
    for (int i = 0; i < count; i++) {
        cJSON *f = cJSON_CreateObject();
        cJSON_AddStringToObject(f, "name", friends[i].name);
        cJSON_AddBoolToObject(f, "focused", friends[i].focused);
        cJSON_AddNumberToObject(f, "pct", friends[i].pct);
        cJSON_AddItemToArray(list, f);
    }
    send_json(o);
}

static const char *phase_name(Phase p)
{
    switch (p) {
        case Phase::STUDY: return "study";
        case Phase::BREAK: return "break";
        case Phase::LONG_REST: return "long_rest";
        default: return "idle";
    }
}

static void send_hello()
{
    cJSON *o = cJSON_CreateObject();
    cJSON_AddStringToObject(o, "t", "hello");
    cJSON_AddStringToObject(o, "fw", POMO_FW_VERSION);
    send_json(o);
}

static void send_state(const SessionSnapshot &s)
{
    cJSON *o = cJSON_CreateObject();
    cJSON_AddStringToObject(o, "t", "state");
    cJSON_AddNumberToObject(o, "left", s.remaining_s);
    cJSON_AddNumberToObject(o, "total", s.total_s);
    cJSON_AddStringToObject(o, "phase", phase_name(s.phase));
    cJSON_AddStringToObject(o, "focus", s.drifting ? "drift" : "focused");
    cJSON_AddNumberToObject(o, "session", s.session_index);
    cJSON_AddStringToObject(o, "drift", s.drift_label);
    cJSON_AddNumberToObject(o, "drift_s", s.drift_s);
    cJSON_AddNumberToObject(o, "mult", s.multiplier);
    cJSON_AddNumberToObject(o, "xp", s.xp);
    cJSON_AddNumberToObject(o, "level", s.level);
    send_json(o);
}

static void send_start(const SessionSnapshot &s)
{
    cJSON *o = cJSON_CreateObject();
    cJSON_AddStringToObject(o, "t", "start");
    cJSON_AddNumberToObject(o, "study", s.study_min);
    cJSON_AddNumberToObject(o, "brk", s.break_min);
    cJSON_AddNumberToObject(o, "session", s.session_index);
    send_json(o);
}

static void send_end(const SessionResult &r, bool early)
{
    cJSON *o = cJSON_CreateObject();
    cJSON_AddStringToObject(o, "t", "end");
    cJSON_AddNumberToObject(o, "pct", r.pct);
    cJSON_AddNumberToObject(o, "xp", r.xp_delta);
    cJSON_AddNumberToObject(o, "level", r.level);
    cJSON_AddNumberToObject(o, "total_xp", r.xp);
    cJSON_AddNumberToObject(o, "focused_min", r.focused_min);
    cJSON_AddNumberToObject(o, "elapsed_min", r.elapsed_min);
    cJSON_AddBoolToObject(o, "penalty", r.penalty);
    cJSON_AddBoolToObject(o, "early", early);
    send_json(o);
}

// ---- Receiving ----

static int lookup(const char *value, const char *const *names, int count)
{
    for (int i = 0; i < count; i++)
        if (strcmp(value, names[i]) == 0) return i;
    return -1;
}

static void handle_config(const cJSON *msg)
{
    static const char *const BODIES[] = {"bot", "cat", "dog"};
    static const char *const FACES[] = {"plain", "monocle"};
    static const char *const FINISHES[] = {"brass", "copper", "pewter", "verdigris"};

    AppState::UiCommand cmd = {};
    cmd.type = AppState::UiCommand::CONFIG;
    cmd.body = cmd.face = cmd.finish = -1;

    const cJSON *user = cJSON_GetObjectItemCaseSensitive(msg, "user");
    if (cJSON_IsString(user)) {
        strncpy(cmd.user, user->valuestring, sizeof(cmd.user) - 1);
        cmd.has_user = true;
    }

    const cJSON *pet = cJSON_GetObjectItemCaseSensitive(msg, "pet");
    if (cJSON_IsObject(pet)) {
        const cJSON *name = cJSON_GetObjectItemCaseSensitive(pet, "name");
        if (cJSON_IsString(name) && name->valuestring[0]) {
            strncpy(cmd.pet_name, name->valuestring, sizeof(cmd.pet_name) - 1);
            cmd.has_pet_name = true;
        }
        const cJSON *body = cJSON_GetObjectItemCaseSensitive(pet, "body");
        const cJSON *face = cJSON_GetObjectItemCaseSensitive(pet, "face");
        const cJSON *color = cJSON_GetObjectItemCaseSensitive(pet, "color");
        if (cJSON_IsString(body)) cmd.body = lookup(body->valuestring, BODIES, PET_BODY_COUNT);
        if (cJSON_IsString(face)) cmd.face = lookup(face->valuestring, FACES, PET_FACE_COUNT);
        if (cJSON_IsString(color)) cmd.finish = lookup(color->valuestring, FINISHES, PET_FINISH_COUNT);
    }

    // level, xp and hearts are ignored: the device owns progress
    const cJSON *time = cJSON_GetObjectItemCaseSensitive(msg, "time");
    if (cJSON_IsNumber(time) && time->valuedouble > 1600000000) {
        cmd.epoch = (long)time->valuedouble;
        cmd.has_time = true;
    }
    const cJSON *tz = cJSON_GetObjectItemCaseSensitive(msg, "tz");
    if (cJSON_IsNumber(tz) && tz->valueint >= -14 * 60 && tz->valueint <= 14 * 60) {
        cmd.tz_minutes = tz->valueint;
        cmd.has_tz = true;
    }
    AppState::instance().post(cmd);
}

static void handle_line(const char *line)
{
    cJSON *msg = cJSON_Parse(line);
    if (!msg) return; // not JSON: ignore
    const cJSON *t = cJSON_GetObjectItemCaseSensitive(msg, "t");
    if (!cJSON_IsString(t)) {
        cJSON_Delete(msg);
        return;
    }

    if (strcmp(t->valuestring, "drift") == 0) {
        const cJSON *active = cJSON_GetObjectItemCaseSensitive(msg, "active");
        const cJSON *label = cJSON_GetObjectItemCaseSensitive(msg, "label");
        const bool on = cJSON_IsTrue(active);
        Session::instance().setDrift(on, cJSON_IsString(label) ? label->valuestring : "Drift");
        ESP_LOGI(TAG, "drift %s", on ? "started" : "ended");
    } else if (strcmp(t->valuestring, "config") == 0) {
        handle_config(msg);
        ESP_LOGI(TAG, "config received");
    } else if (strcmp(t->valuestring, "start") == 0) {
        const cJSON *study = cJSON_GetObjectItemCaseSensitive(msg, "study");
        const cJSON *brk = cJSON_GetObjectItemCaseSensitive(msg, "brk");
        const int s = cJSON_IsNumber(study) ? study->valueint : 25;
        const int b = cJSON_IsNumber(brk) ? brk->valueint : SessionCore::breakMin(SessionCore::snapStudy(s));
        if (!Session::instance().start(s, b)) ESP_LOGI(TAG, "start ignored: a session is running");
    } else if (strcmp(t->valuestring, "peek") == 0) {
        AppState::UiCommand cmd = {};
        cmd.type = AppState::UiCommand::PEEK;
        AppState::instance().post(cmd);
    }
    // Anything else (end, break, finished, ...) is ignored: the device runs the session

    AppState::UiCommand seen = {};
    seen.type = AppState::UiCommand::LINK_SEEN;
    AppState::instance().post(seen);
    cJSON_Delete(msg);
}

void LineAssembler::feed(const char *data, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        const char ch = data[i];
        if (ch == '\n' || ch == '\r') {
            if (!overflow && len > 0) {
                buf[len] = '\0';
                handle_line(buf);
            }
            len = 0;
            overflow = false;
        } else if (len < MAX_LINE) {
            buf[len++] = ch;
        } else {
            overflow = true;
        }
    }
}

// ---- Task ----

static void link_task(void *arg)
{
    static LineAssembler usb;
    char rx[128];
    TickType_t next_state = xTaskGetTickCount();
    Phase last_phase = Phase::IDLE;

    send_hello();
    for (;;) {
        const int n = uart_read_bytes(LINK_UART, rx, sizeof(rx), pdMS_TO_TICKS(100));
        if (n > 0) usb.feed(rx, (size_t)n);

        SessionEvent event;
        while (xQueueReceive(s_events, &event, 0) == pdTRUE) {
            switch (event.type) {
                case SessionEventType::STARTED:
                case SessionEventType::BREAK_ENDED:
                    send_start(Session::instance().snapshot());
                    break;
                case SessionEventType::STUDY_ENDED:
                    send_end(event.result, false);
                    break;
                case SessionEventType::ENDED_EARLY:
                    send_end(event.result, true);
                    break;
                case SessionEventType::SET_ENDED:
                    break;
            }
        }

        // State once a second while a session or break runs, plus once on going idle
        if (xTaskGetTickCount() - next_state < 0x80000000u) {
            next_state += pdMS_TO_TICKS(1000);
            const SessionSnapshot snap = Session::instance().snapshot();
            if (snap.phase != Phase::IDLE || last_phase != Phase::IDLE) send_state(snap);
            last_phase = snap.phase;
        }
    }
}

void link_init()
{
    s_tx_mutex = xSemaphoreCreateMutex();
    s_events = Session::instance().subscribe();
    ESP_ERROR_CHECK(uart_driver_install(LINK_UART, UART_RX_BUF, 0, 0, nullptr, 0));
    xTaskCreate(link_task, "link", 4096, nullptr, 4, nullptr);
    ESP_LOGI(TAG, "USB serial link ready");
}
