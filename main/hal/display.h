#pragma once
#include <stdint.h>
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
esp_err_t init_ili9341_with_lvgl(void);
// Backlight brightness, 0-100 %
void display_set_brightness(uint8_t pct);
#ifdef __cplusplus
}
#endif
