#pragma once
// LVGL includes this from its assert header (CONFIG_LV_ASSERT_HANDLER_INCLUDE). LVGL's own
// handler is `while(1);`: a failed allocation or NULL check would freeze the UI with no log.
// Abort instead, so the device prints the assert and a backtrace, then reboots.
// The top-level CMakeLists.txt puts this folder on every component's include path.
#include <stdlib.h>

#undef LV_ASSERT_HANDLER
#define LV_ASSERT_HANDLER abort();
