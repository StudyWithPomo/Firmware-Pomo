#pragma once
// Freeze checks. The LVGL task calls health_ui_alive() from a 1 s timer. A small watcher task
// logs the heap every minute, and if the UI stops running for 5 s it logs why and aborts:
// the panic handler prints a backtrace and a core dump to the serial monitor, then reboots.

// "[heap] stage: free, largest block, min ever"
void health_log_heap(const char *stage);

// Call from the LVGL task (any lv_timer)
void health_ui_alive();

// Start the watcher (once the UI is up)
void health_start();
