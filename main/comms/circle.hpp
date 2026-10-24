#pragma once
// Study circle (linked play): which Pomo devices are nearby, and what they're doing.
//
// A task runs CircleCore (app/circle_core.hpp) every 500 ms: it builds this device's beacon,
// drops friends that went quiet, and on any change passes the circle to Session::setCircle
// (multiplier and bonus pause) and AppState (lamps, names, the laptop's circle message).
//
// Beacons arrive through circle_on_receive(). The radio (ESP-NOW) comes in milestone L2; until
// then, building with POMO_FAKE_PEER=1 feeds scripted friends through the same path:
//   Ava studies for 100 s (drifting from 40 to 55 s), then goes quiet for 20 s; repeats
//   Dev is always nearby but resting (not in a session): 3× with both, 2× while Ava is quiet
#include <stddef.h>
#include <stdint.h>

#ifndef POMO_FAKE_PEER
#define POMO_FAKE_PEER 0
#endif

// Start the circle task (after Session::init and AppState::attach)
void circle_init();

// The name this device shows in friends' circles (any task, any time)
void circle_set_name(const char *name);

// A packet from another device (any task, including the Wi-Fi task). Non-Pomo packets are
// ignored.
void circle_on_receive(const uint8_t mac[6], const uint8_t *data, size_t len);
