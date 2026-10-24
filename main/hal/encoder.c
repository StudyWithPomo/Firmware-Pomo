#include "encoder.h"

#include <stdatomic.h>
#include <stdbool.h>

#include "board_pins.h"

#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "encoder";

// Quadrature counts per detent (most detented encoders give 4). Flip ENC_REVERSE if
// turning clockwise moves focus backwards.
#define ENC_COUNTS_PER_DETENT 4
#define ENC_REVERSE           0

#define BTN_POLL_MS           10
#define BTN_DEBOUNCE_SAMPLES  3     // 30 ms stable before a change counts
#define BTN_HOLD_MS           2000

static pcnt_unit_handle_t s_pcnt;
static lv_indev_t *s_indev;
static encoder_hold_cb_t s_hold_cb;

static int s_last_count;

// Written by the button poll (esp_timer task), consumed by the LVGL read callback
static atomic_int s_pending_clicks;
static atomic_bool s_pending_hold;

// Button poll state (esp_timer task only)
static bool s_btn_stable_down;
static int s_btn_same_samples;
static int64_t s_btn_down_since_us;
static bool s_btn_hold_fired;

// LVGL read callback state (LVGL task only)
static bool s_click_down;

#if HAS_ENCODER

static void button_poll(void *arg)
{
    const bool raw_down = gpio_get_level(PIN_ENC_BTN) == 0; // active low, pull-up

    if (raw_down != s_btn_stable_down) {
        if (++s_btn_same_samples < BTN_DEBOUNCE_SAMPLES) return;
        s_btn_same_samples = 0;
        s_btn_stable_down = raw_down;

        if (raw_down) {
            s_btn_down_since_us = esp_timer_get_time();
            s_btn_hold_fired = false;
        } else if (!s_btn_hold_fired) {
            atomic_fetch_add(&s_pending_clicks, 1);
        }
        return;
    }
    s_btn_same_samples = 0;

    if (s_btn_stable_down && !s_btn_hold_fired &&
        esp_timer_get_time() - s_btn_down_since_us >= (int64_t)BTN_HOLD_MS * 1000) {
        s_btn_hold_fired = true;
        atomic_store(&s_pending_hold, true);
    }
}

static void run_hold_cb(void *arg)
{
    if (s_hold_cb) s_hold_cb();
}

static void encoder_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    int count = 0;
    if (pcnt_unit_get_count(s_pcnt, &count) == ESP_OK) {
        const int steps = (count - s_last_count) / ENC_COUNTS_PER_DETENT;
        if (steps != 0) {
            s_last_count += steps * ENC_COUNTS_PER_DETENT; // keep the remainder
            data->enc_diff = (int16_t)(ENC_REVERSE ? -steps : steps);
        }
    }

    // A short press is replayed to LVGL as one pressed read followed by one released read
    if (s_click_down) {
        s_click_down = false;
        data->state = LV_INDEV_STATE_RELEASED;
    } else if (atomic_load(&s_pending_clicks) > 0) {
        atomic_fetch_sub(&s_pending_clicks, 1);
        s_click_down = true;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    // Deferred so the hold handler can rebuild the screen outside indev processing
    if (atomic_exchange(&s_pending_hold, false)) {
        lv_async_call(run_hold_cb, NULL);
    }
}

static esp_err_t pcnt_init(void)
{
    const pcnt_unit_config_t unit_cfg = {
        .low_limit = -1000,
        .high_limit = 1000,
        .flags.accum_count = true,
    };
    ESP_RETURN_ON_ERROR(pcnt_new_unit(&unit_cfg, &s_pcnt), TAG, "pcnt unit");

    const pcnt_glitch_filter_config_t filter_cfg = { .max_glitch_ns = 1000 };
    ESP_RETURN_ON_ERROR(pcnt_unit_set_glitch_filter(s_pcnt, &filter_cfg), TAG, "glitch filter");

    // Full x4 quadrature: each channel counts edges on one pin, direction from the other
    pcnt_channel_handle_t chan_a, chan_b;
    const pcnt_chan_config_t a_cfg = { .edge_gpio_num = PIN_ENC_A, .level_gpio_num = PIN_ENC_B };
    const pcnt_chan_config_t b_cfg = { .edge_gpio_num = PIN_ENC_B, .level_gpio_num = PIN_ENC_A };
    ESP_RETURN_ON_ERROR(pcnt_new_channel(s_pcnt, &a_cfg, &chan_a), TAG, "channel A");
    ESP_RETURN_ON_ERROR(pcnt_new_channel(s_pcnt, &b_cfg, &chan_b), TAG, "channel B");
    pcnt_channel_set_edge_action(chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
    pcnt_channel_set_level_action(chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    pcnt_channel_set_edge_action(chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    pcnt_channel_set_level_action(chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

    gpio_set_pull_mode(PIN_ENC_A, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(PIN_ENC_B, GPIO_PULLUP_ONLY);

    // accum_count needs watch points at the limits to fold overflows into the count
    ESP_RETURN_ON_ERROR(pcnt_unit_add_watch_point(s_pcnt, unit_cfg.high_limit), TAG, "watch high");
    ESP_RETURN_ON_ERROR(pcnt_unit_add_watch_point(s_pcnt, unit_cfg.low_limit), TAG, "watch low");

    ESP_RETURN_ON_ERROR(pcnt_unit_enable(s_pcnt), TAG, "enable");
    ESP_RETURN_ON_ERROR(pcnt_unit_clear_count(s_pcnt), TAG, "clear");
    ESP_RETURN_ON_ERROR(pcnt_unit_start(s_pcnt), TAG, "start");
    return ESP_OK;
}

static esp_err_t button_init(void)
{
    const gpio_config_t btn_cfg = {
        .pin_bit_mask = 1ULL << PIN_ENC_BTN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&btn_cfg), TAG, "button gpio");

    const esp_timer_create_args_t timer_args = {
        .callback = button_poll,
        .name = "enc_btn",
    };
    esp_timer_handle_t timer;
    ESP_RETURN_ON_ERROR(esp_timer_create(&timer_args, &timer), TAG, "button timer");
    ESP_RETURN_ON_ERROR(esp_timer_start_periodic(timer, BTN_POLL_MS * 1000), TAG, "button timer start");
    return ESP_OK;
}
#endif

esp_err_t encoder_init(void)
{
#if HAS_ENCODER
    ESP_RETURN_ON_ERROR(pcnt_init(), TAG, "pcnt");
    ESP_RETURN_ON_ERROR(button_init(), TAG, "button");

    s_indev = lv_indev_create();
    lv_indev_set_type(s_indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(s_indev, encoder_read);

    ESP_LOGI(TAG, "Encoder on A=%d B=%d BTN=%d", PIN_ENC_A, PIN_ENC_B, PIN_ENC_BTN);
    return ESP_OK;
#else
    // Boards without an encoder: nothing to set up, no PCNT unit or button
    // poll timer to leave running.
    return ESP_OK;
#endif
}

lv_indev_t *encoder_get_indev(void)
{
#if !HAS_ENCODER
    return NULL;
#endif
    return s_indev;
}

void encoder_set_hold_cb(encoder_hold_cb_t cb)
{
    s_hold_cb = cb;
}
