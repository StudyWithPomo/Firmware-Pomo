#pragma once
// Laptop link: one JSON object per line, 115200 baud, UTF-8.
//
// Every transport (USB serial now, Bluetooth SPP later) feeds bytes into its own
// LineAssembler, and complete lines go to one handler. Outgoing messages are written to
// every transport. Bad or unknown lines are ignored, so ESP_LOG output on the same UART is
// harmless to the laptop.
//
// Laptop -> device:
//   {"t":"drift","active":true,"label":"YouTube","kind":"site"}   {"t":"drift","active":false}
//   {"t":"config","user":"Sidak","pet":{"name":"Mochi","body":"bot","face":"plain","color":"brass"},
//    "time":1792790040,"tz":-240}        time = epoch seconds, tz = minutes east of UTC (both optional)
//   {"t":"start","study":25,"brk":5}     starts a session if none is running (values snapped)
//   {"t":"peek"}                         lights the timer for 5 s
// Device -> laptop:
//   {"t":"hello","fw":"..."}
//   {"t":"start","study":25,"brk":5,"session":1}
//   {"t":"state","left":522,"total":1500,"phase":"study","focus":"focused","session":1,
//    "drift":"","drift_s":0,"mult":2,"xp":340,"level":1}            every second while not idle
//   {"t":"end","pct":90,"xp":44,"level":3,"total_xp":1044,"focused_min":22,"elapsed_min":25,
//    "penalty":false,"early":false}
//   {"t":"circle","friends":[{"name":"Ava","focused":true,"pct":0}]}
//   {"t":"knob","ev":"press"}            the user peeked on the device
#include <stddef.h>

// Start the USB serial link task (after Session::init and AppState::attach)
void link_init();

// Send one JSON line to every connected transport. Any task.
void link_send(const char *json);

// Convenience senders (any task)
void link_send_knob(const char *ev);
void link_send_circle();   // from AppState's friends; call from the LVGL task

// Collects bytes into lines for one transport; complete lines go to the shared handler
class LineAssembler {
public:
    void feed(const char *data, size_t len);
private:
    static const size_t MAX_LINE = 256;
    char buf[MAX_LINE + 1];
    size_t len = 0;
    bool overflow = false; // drop the rest of an over-long line
};
