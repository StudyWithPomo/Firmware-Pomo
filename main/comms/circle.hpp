#pragma once
// Study circle (linked play): which Pomo devices are nearby, and studying together by invite.
//
// A task runs CircleCore (app/circle_core.hpp, where the rules are) every 500 ms: it sends
// this device's beacon, invites and declines, drops friends that went quiet, starts the
// session when the group does, and on any change passes the circle to Session::setCircle
// (multiplier and bonus pause) and AppState (the Circle and Invite screens, lamps, the
// laptop's circle message).
//
// The radio is ESP-NOW broadcast on Wi-Fi channel 1, with no router: every message goes to
// every Pomo in range, and messages arrive through circle_on_receive(). To test on one board,
// building with POMO_FAKE_PEER=1 also adds two scripted friends, through the same path:
//   Ava is free until invited; she accepts after 2 s and then keeps in step with this
//   device, drifting 40-55 s into every 2 minutes while studying
//   Dev declines invites after 2 s, and invites this device himself 20 s after boot and
//   every 3 minutes after that while it's free (accept, and he starts right away)
#include <stddef.h>
#include <stdint.h>

#ifndef POMO_FAKE_PEER
#define POMO_FAKE_PEER 0
#endif

// Start Wi-Fi, ESP-NOW and the circle task (after Session::init, AppState::attach and
// Bluetooth). If the radio fails it logs why and the circle runs without it.
void circle_init();

// The name this device shows in friends' circles (any task, any time)
void circle_set_name(const char *name);

// A packet from another device (any task, including the Wi-Fi task). Non-Pomo packets are
// ignored.
void circle_on_receive(const uint8_t mac[6], const uint8_t *data, size_t len);

// From the UI (any task). The circle task carries them out within 500 ms, and the result
// comes back through AppState's friends and CircleView.
void circle_invite(const uint8_t mac[6], int study, int brk, bool test); // a free friend
void circle_start();                // host: start now with whoever joined
void circle_cancel();               // host: close the group before it starts
void circle_answer(bool accept);    // guest: the invite on screen (false also leaves a group
                                    // that hasn't started)
