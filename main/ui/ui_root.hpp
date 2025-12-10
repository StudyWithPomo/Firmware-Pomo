#pragma once
#include "lvgl.h"

// Initialize the root UI components (System Bar and Content Area)
void ui_root_init();

// Get the content area object
lv_obj_t *ui_get_content_area();

// Toggle displaying time
void ui_toggle_time_display(bool enable);