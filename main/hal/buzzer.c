#include "buzzer.h"

#include <stdbool.h>

#include "board.h"

#include "driver/ledc.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "buzzer";

// LEDC timer 0 / channel 0 are the backlight's (display.c)
#define BZ_LEDC_MODE     LEDC_LOW_SPEED_MODE
#define BZ_LEDC_TIMER    LEDC_TIMER_1
#define BZ_LEDC_CHANNEL  LEDC_CHANNEL_1
#define BZ_LEDC_RES      LEDC_TIMER_10_BIT

typedef struct {
    const buzz_note_t *notes;
    uint8_t count;
    uint8_t prio;
    uint8_t repeat;
} buzz_pattern_t;

#define BUZZ_NOTES(name, prio, repeat, ...) static const buzz_note_t notes_##name[] = { __VA_ARGS__ };
BUZZ_PATTERNS(BUZZ_NOTES)
#undef BUZZ_NOTES

#define BUZZ_ENTRY(name, prio, repeat, ...) \
    [BUZZ_##name] = { notes_##name, sizeof(notes_##name) / sizeof(notes_##name[0]), prio, repeat },
static const buzz_pattern_t s_patterns[BUZZ_COUNT] = { BUZZ_PATTERNS(BUZZ_ENTRY) };
#undef BUZZ_ENTRY

static SemaphoreHandle_t s_lock;
static esp_timer_handle_t s_timer;
static bool s_ready;

// What's playing (NULL when quiet), the next note and which round of `repeat` this is
static const buzz_pattern_t *s_pattern;
static int s_step;
static int s_round;

static void tone(uint16_t hz, uint8_t vol)
{
    if (hz && vol) {
        if (vol > 50) vol = 50;
        ledc_set_freq(BZ_LEDC_MODE, BZ_LEDC_TIMER, hz);
        ledc_set_duty(BZ_LEDC_MODE, BZ_LEDC_CHANNEL, ((1u << BZ_LEDC_RES) * vol) / 100);
    } else {
        ledc_set_duty(BZ_LEDC_MODE, BZ_LEDC_CHANNEL, 0);
    }
    ledc_update_duty(BZ_LEDC_MODE, BZ_LEDC_CHANNEL);
}

// Start the next note and arm the timer for its end. Call with s_lock held.
static void next_note_locked(void)
{
    if (!s_pattern) return;
    if (s_step >= s_pattern->count) {
        if (++s_round >= s_pattern->repeat) {
            s_pattern = NULL;
            tone(0, 0);
            return;
        }
        s_step = 0;
    }
    const buzz_note_t *note = &s_pattern->notes[s_step++];
    tone(note->hz, note->vol);
    esp_timer_start_once(s_timer, (uint64_t)note->ms * 1000);
}

static void on_note_end(void *arg)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    // buzzer_play() may have re-armed the timer while this call waited for the lock: then
    // the new pattern's first note is still playing, so leave it alone
    if (!esp_timer_is_active(s_timer)) next_note_locked();
    xSemaphoreGive(s_lock);
}

esp_err_t buzzer_init(void)
{
    const int pin = board()->buzzer;
    if (pin == PIN_NONE) return ESP_OK;

    const ledc_timer_config_t timer_cfg = {
        .speed_mode = BZ_LEDC_MODE,
        .duty_resolution = BZ_LEDC_RES,
        .timer_num = BZ_LEDC_TIMER,
        .freq_hz = 2700,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_cfg), TAG, "timer");

    const ledc_channel_config_t channel_cfg = {
        .gpio_num = pin,
        .speed_mode = BZ_LEDC_MODE,
        .channel = BZ_LEDC_CHANNEL,
        .timer_sel = BZ_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&channel_cfg), TAG, "channel");

    s_lock = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(s_lock, ESP_ERR_NO_MEM, TAG, "lock");
    const esp_timer_create_args_t args = { .callback = on_note_end, .name = "buzzer" };
    ESP_RETURN_ON_ERROR(esp_timer_create(&args, &s_timer), TAG, "esp_timer");

    s_ready = true;
    ESP_LOGI(TAG, "on GPIO %d", pin);
    return ESP_OK;
}

void buzzer_play(buzz_t sound)
{
    if (!s_ready || sound >= BUZZ_COUNT) return;
    const buzz_pattern_t *p = &s_patterns[sound];

    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (!s_pattern || p->prio >= s_pattern->prio) {
        esp_timer_stop(s_timer);
        s_pattern = p;
        s_step = 0;
        s_round = 0;
        next_note_locked();
    }
    xSemaphoreGive(s_lock);
}
