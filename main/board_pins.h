#pragma once
// Every GPIO the firmware uses, in one place.
// Both demo devices run the same firmware; per-device options are the HAS_* flags below.

// ---- Display: ILI9341 on SPI2 (HSPI) ----
// GPIO 12 (DC) is a strapping pin: if it's pulled high at boot the ESP32 sets flash to 1.8 V
// and won't boot. If boot is flaky, move DC to a free pin.
#define PIN_LCD_MOSI    23
#define PIN_LCD_CLK     18
#define PIN_LCD_CS      5
#define PIN_LCD_DC      17
#define PIN_LCD_RST     16
#define PIN_LCD_BL      21

// ---- Touch: XPT2046 on SPI3 (VSPI) ----
#define PIN_TOUCH_MOSI  23
#define PIN_TOUCH_MISO  19
#define PIN_TOUCH_CLK   18
#define PIN_TOUCH_CS    4
#define PIN_TOUCH_IRQ   27

// ---- Rotary encoder ----
// TODO-confirm: placeholders until the real wiring is known
#define PIN_ENC_A       25
#define PIN_ENC_B       33
#define PIN_ENC_BTN     32

// ---- Buzzer (passive, LEDC PWM) ----
// TODO-confirm: placeholder
#define HAS_BUZZER      1
#define PIN_BUZZER      22

// ---- Battery gauge (Device A only), ADC1 through a voltage divider ----
// TODO-confirm: placeholder. GPIO 34/35 are input-only, which suits the ADC.
#ifndef HAS_BATTERY
#define HAS_BATTERY     0
#endif
#define PIN_BATTERY_ADC 34

// ---- Status lamp: not fitted yet ----
#define HAS_LAMP        0
