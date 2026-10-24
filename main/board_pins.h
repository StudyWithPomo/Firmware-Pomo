#pragma once
// Every GPIO the firmware uses, in one place.
// Both demo devices run the same firmware; per-device options are the HAS_* flags below.

// ---- Shared SPI bus: the display and the touch controller ----
// Both sit on one bus (MOSI 23, CLK 18, MISO 19) with their own chip selects. These are
// VSPI's native pins, so the bus runs on SPI3 through the IO MUX at full speed.
#define BOARD_SPI_HOST  SPI3_HOST   // needs driver/spi_common.h
#define PIN_SPI_MOSI    23
#define PIN_SPI_CLK     18
#define PIN_SPI_MISO    19          // only the touch controller answers

// ---- Display: ILI9341 ----
// GPIO 5 (CS) is a strapping pin, but it's only sampled at reset, where its internal pull-up
// holds it high, as a chip select wants anyway.
#define PIN_LCD_MOSI    23
#define PIN_LCD_CLK     18
#define PIN_LCD_CS      5
#define PIN_LCD_DC      17
#define PIN_LCD_RST     16
#define PIN_LCD_BL      21

// ---- Touch: XPT2046 (same bus as the display) ----
#define PIN_TOUCH_MOSI  23
#define PIN_TOUCH_MISO  19
#define PIN_TOUCH_CLK   18
#define PIN_TOUCH_CS    4
#define PIN_TOUCH_IRQ   27

// ---- Rotary encoder (test board one) ----
#define PIN_ENC_A       25
#define PIN_ENC_B       33
#define PIN_ENC_BTN     32

// ---- Buzzer (passive, LEDC PWM, test board one) ----
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

// config for BOARD #2 (compact)
/*
// ---- Display: ILI9341 on VSPI (shared with touch) ----
#define PIN_LCD_MOSI    23
#define PIN_LCD_CLK     18
#define PIN_LCD_CS      5
#define PIN_LCD_DC      21
#define PIN_LCD_RST     22
// Backlight tied to 3V3, no GPIO

// ---- Touch: XPT2046 on the same bus ----
#define PIN_TOUCH_MOSI  23
#define PIN_TOUCH_MISO  19
#define PIN_TOUCH_CLK   18
#define PIN_TOUCH_CS    4
#define PIN_TOUCH_IRQ   15

// ---- Buzzer (passive, LEDC PWM) ----
#define HAS_BUZZER      1
#define PIN_BUZZER      2

NO ROTARY ENCODER
*/