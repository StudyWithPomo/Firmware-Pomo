#pragma once
// Bluetooth Classic SPP: the laptop pairs with "Pomo-XXXX" and gets a serial port that
// carries the same JSON lines as USB. Incoming bytes go through their own LineAssembler to
// the shared link handler; link_send() writes to SPP whenever a laptop is connected.
#include <stddef.h>

#include "esp_err.h"

// Start the controller, Bluedroid and an SPP server (needs NVS initialized)
esp_err_t bt_spp_init();

// Write bytes to the connected laptop. Returns false if nobody is connected. Any task.
bool bt_spp_send(const char *data, size_t len);

bool bt_spp_connected();

// True once after each new connection (the link task then sends hello)
bool bt_spp_take_new_connection();
