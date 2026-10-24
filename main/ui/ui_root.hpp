#pragma once
#include "lvgl.h"

// Initialize the root UI (black screen + full-size content area)
void ui_root_init();

// Get the content area object that screens are built into
lv_obj_t *ui_get_content_area();
