#include "board.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

static const char *TAG = "board";

// Test board one: rotary encoder, PWM backlight
static const board_t BOARD_1 = {
    .id = 1,
    .lcd_cs = 5, .lcd_dc = 17, .lcd_rst = 16, .lcd_bl = 21,
    .touch_cs = 4, .touch_irq = 27,
    .touch_flip = false,
    .has_encoder = true,
    .enc_a = 25, .enc_b = 33, .enc_btn = 32,
    .buzzer = 22,
    // TODO-confirm: placeholder on GPIO 34 (input-only, which suits the ADC), not fitted
    .battery_adc = PIN_NONE,
};

// Compact board: no encoder, backlight tied to 3V3
static const board_t BOARD_2 = {
    .id = 2,
    .lcd_cs = 5, .lcd_dc = 21, .lcd_rst = 22, .lcd_bl = PIN_NONE,
    .touch_cs = 4, .touch_irq = 15,
    .touch_flip = false,
    .has_encoder = false,
    .enc_a = PIN_NONE, .enc_b = PIN_NONE, .enc_btn = PIN_NONE,
    .buzzer = 2,
    .battery_adc = PIN_NONE,
};

static board_t s_board = BOARD_2;

typedef enum { CFG_OPEN, CFG_GND, CFG_3V3 } cfg_level_t;

static const char *level_name(cfg_level_t level)
{
    switch (level) {
        case CFG_GND: return "GND";
        case CFG_3V3: return "3V3";
        default: return "open";
    }
}

// The internal pulls are ~45 kΩ, so 50 µs is plenty for an open pin to settle
static int read_with_pull(int pin, gpio_pull_mode_t pull)
{
    gpio_set_pull_mode(pin, pull);
    esp_rom_delay_us(50);
    return gpio_get_level(pin);
}

static cfg_level_t read_cfg(int pin)
{
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    const int with_up = read_with_pull(pin, GPIO_PULLUP_ONLY);
    const int with_down = read_with_pull(pin, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(pin, GPIO_FLOATING); // no current through a pull into a wired pin

    if (with_up == with_down) return with_up ? CFG_3V3 : CFG_GND;
    return CFG_OPEN; // followed the pulls
}

void board_detect(void)
{
    const cfg_level_t a = read_cfg(PIN_CFG_A);
    const cfg_level_t b = read_cfg(PIN_CFG_B);

    if (a == CFG_OPEN) {
        s_board = BOARD_2;
    } else {
        s_board = BOARD_1;
        s_board.has_encoder = a == CFG_GND;
        s_board.touch_flip = b == CFG_GND;
        if (b == CFG_3V3) ESP_LOGW(TAG, "CFG B (GPIO %d) on 3V3 means nothing yet: regular touch", PIN_CFG_B);
    }

    ESP_LOGI(TAG, "CFG A (GPIO %d) %s, CFG B (GPIO %d) %s -> board %d, %s, %s touch", PIN_CFG_A, level_name(a),
             PIN_CFG_B, level_name(b), s_board.id, s_board.has_encoder ? "encoder" : "no encoder",
             s_board.touch_flip ? "flipped" : "regular");
}

const board_t *board(void)
{
    return &s_board;
}
