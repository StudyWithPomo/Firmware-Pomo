#pragma once
#include "esp_err.h"
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif

// Rotary encoder with push button, registered as an LVGL encoder input.
//   Turn          -> moves focus in the active lv_group (or changes a value in edit mode)
//   Short press   -> LVGL click, sent on release so a hold never also counts as a click
//   Hold >= 2 s   -> the hold callback, run in the LVGL task
// Call with the LVGL lock held.
esp_err_t encoder_init(void);

lv_indev_t *encoder_get_indev(void);

// Called in the LVGL task when the button has been held for 2 s
typedef void (*encoder_hold_cb_t)(void);
void encoder_set_hold_cb(encoder_hold_cb_t cb);

#ifdef __cplusplus
}
#endif
