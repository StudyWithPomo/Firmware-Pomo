#pragma once
// Every GPIO the firmware uses, in one place.
//
// One firmware runs on both demo boards. At boot board_detect() reads two config pins
// and picks the pinout and options:
//
//   CFG A (GPIO 13)   unconnected -> board 2: compact, no encoder, regular touch
//                                    (CFG B is ignored; board 2 has no room for wires)
//                     GND         -> board 1 with the rotary encoder
//                     3V3         -> board 1 without the encoder
//   CFG B (GPIO 26)   unconnected -> regular touch          (board 1 only)
//                     GND         -> touch flipped on X and Y
//                     3V3         -> spare: treated as regular touch for now
//
// Each config pin is read twice, with the internal pull-up and then the pull-down: a pin
// wired to GND or 3V3 ignores the pull, an unconnected one follows it. So the pins need no
// resistors, only a wire. 13 and 26 are the only GPIOs free on both boards that have both
// pulls and no boot-time role (0, 2, 12 and 15 are strapping pins, 14 toggles at boot,
// 34-39 have no pulls).
#include <stdbool.h>

#include "driver/spi_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---- Config pins ----
#define PIN_CFG_A       13
#define PIN_CFG_B       26

#define PIN_NONE        (-1)

// ---- Shared SPI bus: the display and the touch controller, on every board ----
// Both sit on one bus (MOSI 23, CLK 18, MISO 19) with their own chip selects. These are
// VSPI's native pins, so the bus runs on SPI3 through the IO MUX at full speed.
#define BOARD_SPI_HOST  SPI3_HOST
#define PIN_SPI_MOSI    23
#define PIN_SPI_CLK     18
#define PIN_SPI_MISO    19          // only the touch controller answers

typedef struct {
    int id;                 // 1 (test board one) or 2 (compact)

    // Display: ILI9341. GPIO 5 (CS) is a strapping pin, but it's only sampled at reset,
    // where its internal pull-up holds it high, as a chip select wants anyway.
    int lcd_cs, lcd_dc, lcd_rst;
    int lcd_bl;             // LEDC PWM backlight; PIN_NONE when it's tied to 3V3

    // Touch: XPT2046 on the shared bus
    int touch_cs, touch_irq;
    bool touch_flip;        // mirror both axes (one of the two displays is mounted flipped)

    // Rotary encoder with push button (board 1 only)
    bool has_encoder;
    int enc_a, enc_b, enc_btn;

    int buzzer;             // passive, LEDC PWM (hal/buzzer.c)
    int battery_adc;        // ADC1 through a divider; PIN_NONE when not fitted
} board_t;

// Read the config pins and pick the board. Call first thing in app_main, before any driver.
void board_detect(void);

// The board found by board_detect() (board 2 before it runs)
const board_t *board(void);

#ifdef __cplusplus
}
#endif
