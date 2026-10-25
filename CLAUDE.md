# Pomo firmware

Firmware for **Study Buddy** (SBU Hacks): a steampunk clockwork pet on an ESP32 that runs a hidden Pomodoro timer, owns XP and level, and reacts when the laptop dashboard ([StudyWithPomo/Frontend-Pomo](https://github.com/StudyWithPomo/Frontend-Pomo)) reports a drift. Built on Cube OS v1 alpha (ESP-IDF + LVGL), now almost entirely rewritten.

Specs, in order of authority:
1. The user's words in the session.
2. The build brief, [docs/STUDY_BUDDY_FIRMWARE.md](docs/STUDY_BUDDY_FIRMWARE.md). Its pin map and some screen details are out of date (see below), but its rules hold: no hearts as a stat (health, lives, pet death; floating hearts when you pet the pet are fine, the user asked for them), the device is the source of truth, ask before changing pins, the partition table, or the LVGL/IDF versions.
3. The design canvas "Study Buddy UI", board "Device screens · steampunk". It is drawn at 2×: halve every size.
4. Frontend-Pomo `pet.js` (pet data), and its README and `schema/pomo-device.schema.json` (protocol).

## Build, flash, test

- **ESP-IDF v6.0.2** at `C:\esp\v6.0.2\esp-idf`, tools in `C:\Espressif`.
  - Use the "ESP-IDF" terminal, or load the environment with `. C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1`.
  - `idf.py -p COM5 flash monitor`. The USB serial port is COM5 (CP210x).
  - The board's auto-reset into download mode doesn't work: **hold BOOT** while it says `Connecting...`. Exit the monitor with **Ctrl+]**.
- **Builds:**
  - **The user runs `idf.py build` and flashing themselves.** Don't run them; say when the code is ready. Rules tests and the PC preview (own build folder) are fine to run.
  - **Never run two builds at once** in the same `build/` folder (the user often runs one in their own terminal). They corrupt each other.
  - Any `sdkconfig` change rebuilds all ~1,568 ESP-IDF files (3–5 min).
  - `sdkconfig` is gitignored and generated from `sdkconfig.defaults`. Delete `sdkconfig` after changing the defaults.
- **Rules tests** for the session/XP logic (`main/app/session_core.cpp`, no IDF needed). MSYS2 g++ is at `/c/msys64/mingw64/bin`:
  `g++ -std=c++17 -Wall -I main test/session_test.cpp main/app/session_core.cpp -o session_test && ./session_test`
  - Pet care (gears, hunger): `g++ -std=c++17 -Wall -I main test/pet_care_test.cpp main/app/pet_care.cpp main/app/session_core.cpp main/utilities.cpp -o pet_care_test && ./pet_care_test`
- **`docs/PET_PLAY_AND_CARE.md` is for the frontend dev**, who recreates the pet's play and care on the dashboard. Keep it exact (numbers, strings, art, sounds, protocol) and add a dated log entry for every change to petting, the Playground, gears or hunger.
- **Test timing on the device:** the "Test" chip in the Study screen's header makes every study session, break and long rest 30 s (scoring minutes are 10 s, a drift minute is 3 s of drift). Focus and Intermission say "· Test". Test sessions earn XP and count in the session log (sessions, sets, today) but add no focused minutes. It isn't saved, so a restart turns it off. Rules: `SessionCore::TEST_*`.
- **PC screen preview:** `tools/preview/build.sh`. Needs one `idf.py build` first.
  - It compiles the real UI and session code against LVGL, using the device's own LVGL config, and renders every screen to `tools/preview/out/*.png`. It also prints LVGL memory per screen.
  - New firmware symbols that screens or AppState call need stubs in `tools/preview/sim_stubs.cpp`. MinGW gaps (strlcat, strlcpy, setenv, localtime_r) are in `stubs/compat.h`.
  - Use it to check layouts before asking the user to flash.
- **Bluetooth:**
  - The device shows up as **Pomo-8126** (Classic SPP, Just Works; PIN 1234 if asked).
  - On the user's laptop the **outgoing** Bluetooth COM port is **COM7** (COM6 is incoming).
  - Windows shows "Not connected" until a program opens the port; that's normal.
- **Debugging crashes and freezes:**
  - Get the full `Backtrace:` line, and the core dump that follows a panic (`idf.py monitor` decodes it).
  - Freeze checks (`system/health.*`, `sdkconfig.defaults`): LVGL asserts abort (`main/lvgl_hooks/pomo_lv_assert.h`; LVGL's default was a silent `while(1);`), the task watchdog panics, LVGL warnings are logged, and a watcher logs `[ui] stalled ...` and aborts when the UI task hasn't run for 5 s. `[heap] every minute` lines show leaks.
  - `POMO_SOAK=N tools/preview/build/pomo_preview <out>` runs every screen and a test-timing set N times and prints LVGL memory, to catch UI leaks (none at 200 rounds as of milestone C).
  - The boot log prints `[heap]` and `[stack] LVGL task` lines.
  - The user's local `sdkconfig` currently has diagnostics on (`CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK`, `CONFIG_HEAP_POISONING_COMPREHENSIVE`). Turn them off during a build that is a full rebuild anyway.

## Hardware (`main/board.h`, `main/board.c`)

Original ESP32 (needs Classic BT; NodeMCU-32S, no PSRAM), 2.8" ILI9341 used landscape 320×240, XPT2046 resistive touch, passive buzzer. One firmware for both demo boards: at boot `board_detect()` reads two config pins, each in three states (wired to GND, wired to 3V3, or unconnected, told apart by reading with the pull-up and then the pull-down). Drivers read their pins from `board()`.

| CFG A (GPIO 13) | | CFG B (GPIO 26), both boards | |
|---|---|---|---|
| unconnected | board 2, no encoder, regular touch | unconnected | regular touch |
| GND | board 1 with encoder | GND | touch flipped on X and Y |
| 3V3 | board 1 without encoder | 3V3 | spare (regular for now) |

Board 2 has no room for config wires, so "nothing wired" is board 2. The user's test board one needs GPIO 13 to GND.

| Signal | Board 1 (test board one) | Board 2 (compact) |
|---|---|---|
| Shared SPI3 bus: MOSI / CLK / MISO | 23 / 18 / 19 | 23 / 18 / 19 |
| LCD CS / DC / RST | 5 / 17 / 16 | 5 / 21 / 22 |
| LCD backlight | 21 (PWM) | tied to 3V3 |
| Touch CS / IRQ | 4 / 27 | 4 / 15 |
| Encoder A / B / button | 25 / 33 / 32 (if CFG A is GND) | none |
| Buzzer (passive, LEDC timer 1 / channel 1) | 22 | 2 |
| Battery ADC | 34 placeholder, not fitted | none |

Display and touch share one SPI bus: `display.c` initializes it, `touch.c` joins as a second device. Pins were set by XtraCube (teammate). Ask before changing pins. Free on both boards: 0, 12, 14, 35, 36, 39 (all with caveats: strapping, boot toggling, or no pulls).

## Architecture

- `main/main.cpp`: start-up order, then logs heap:
  0. `board_detect()` (config pins)
  1. NVS
  2. display
  3. touch
  4. Session
  5. under the LVGL lock: encoder, theme, ui_root, Navigator, AppState, load saved state, Boot screen
  6. the laptop link
  7. Bluetooth
- `hal/`: C drivers.
  - `display.c` handles the ILI9341, landscape via hardware rotation, PWM backlight `display_set_brightness()`, and the **16 KB LVGL task stack**.
  - `touch.c` handles the XPT2046 and calibrates raw readings.
  - `encoder.c` reads the knob with PCNT x4 quadrature. A short press reaches LVGL as a click on release; a 2 s hold calls a hold callback.
  - `buzzer.*`: every sound is a note table in `BUZZ_PATTERNS` in `buzzer.h` (pitch, volume as PWM duty 0-50, ms, repeats, priority), played by an `esp_timer`. A sound only interrupts one of the same or lower priority (feedback < pet < notice < alarm). `buzzer_play()` is safe from any task.
    - Feedback: `theme::focusable()` ticks on every press (touch or knob), Back has its own, −/+ a lighter one, each knob detent a faint one (in `encoder.c`).
    - Pet: `pet_hearts()` coos, a stroke purrs, the Focus "shh" a low hum. Playground: a ratchet click per wind, a music-box tune for the dance, a swish and a crunch for a gear.
    - Timer (in `AppState::handleEvent`): session start, loud chime when study ends (bigger at the long rest), alarm-clock beeps when the break runs out, falling chime when the long rest ends the set, soft falling notes when ended by hand. Also a nudge when a drift starts and a ding-dong for a circle invite.
- `app/`:
  - `session_core.*`: pure C++ rules, unit tested. A break running out goes to `Phase::READY` (event `BREAK_OVER`) and waits; `endBreak()` starts the next session, `endSet()` finishes. The long rest running out still ends the set.
  - `session.*`: a 1 s `esp_timer`, a mutex, a snapshot queue and event queues (`subscribe()` gives each listener its own).
  - `pet_care.*`: pure C++ rules, unit tested: gears (1 per completed session, +2 for the set, max 12), feeding (costs a gear; refused when empty or after 3 in 30 min), hunger from time since fed (full < 30 min, content < 12 h, peckish < 24 h, hungry). Hunger only moves once the laptop has sent the time. Spec: `docs/PET_PLAY_AND_CARE.md`.
  - `storage.*`: NVS namespace `pomo`; only changed keys are written. Includes the session log (`SessionLog` in `session_core`: sessions, sets, focused minutes, today).
- `comms/`:
  - `link.*`: JSON lines over UART0, per-transport `LineAssembler`, cJSON. Pet care goes out as a `pet` message; the laptop's `{"t":"feed"}` runs the device's own feed (spec: `docs/PET_PLAY_AND_CARE.md`). Every line from the laptop calls `Session::laptopHeard()`; the dashboard's `ping` (every 2 s) arms the heartbeat. After 6 s of silence the laptop's drift is dropped and a study session pauses (`state.paused`, Focus shows "Laptop Lost" with **Go on alone**) until the laptop is heard again. A dashboard that never pings never pauses anything.
  - `bt_spp.*`: Bluedroid Classic SPP feeding the same handler.
- `ui/`:
  - `app_state.*`: all `lv_subject`s. It drains the session queues and a `UiCommand` queue in a 50 ms `lv_timer`, navigates on session events, and autosaves 2 s after a change (XP immediately when a session is scored).
  - `navigator.*`: screen registry, a fresh `lv_group` per screen bound to the encoder, `back()` history, per-screen hold handler, screen swaps deferred with `lv_async_call`.
  - `theme.*`: colour tokens and builders (frame, `header()` with ◂ Back, porthole, bubble, chip, button, list_row, `step_buttons()` −/+, nixie tubes, lamps, focus and grabbed styles, `from_knob()`, `ignore_hold`). Petting: `pettable(pet, porthole)` on every pet outside Focus (tap: `pet_hearts()`, hearts and a hop on the top layer; dragging ~30 px: `pet_stroke()`, a squish, one heart, a purr).
  - `pet/`: the `pet_data.h` port of `pet.js`, and a `pet_view` drawn as scaled rectangles in a draw callback. `pet_view_set_squash()` squeezes or stretches it sideways and mirrors it (the spin and the squish, no image buffer); `pet_view_set_key()` puts a turning wind-up key on any body (Playground).
  - `fonts/`: IM Fell English SC, Libre Caslon, IBM Plex Mono, generated with lv_font_conv, with ▸ ◂ − merged from DejaVu Sans. See `fonts/README.md`.
  - `screens/`: one file per screen. Boot, Home (Start + Study/Pet/Circle tiles), Study, Focus, Intermission (Start now / End set), Ready ("Ready for session N of 4?"), Circle, Pet hub, Status, Wardrobe, Playground (wind the key, feed gears).

**UI conventions (from milestone A):**
- Every screen but Home and Focus has `header()`'s ◂ Back, added to the group last so it's the knob's last stop. Hold also goes back; Back with no history goes Home.
- Knob: turn moves, a press on a value row grabs it (group editing, verdigris outline), turning changes it, another press lets go. Touch: a tap only selects a row (`from_knob()` is false), −/+ change it.
- Rows clip their children, so a focus outline needs ~4 px of padding in the row that holds it (`OVERFLOW_VISIBLE` doesn't help).

**Rules that matter:**
- **LVGL is not thread-safe.** Only the LVGL task touches LVGL. Other tasks post to queues (`Session`, `AppState::post`). Any LVGL call outside it needs `lvgl_port_lock`.
- **Keep the object tree shallow.** LVGL redraws recursively at about 0.5 KB of stack per nesting level. A 7 KB stack crashed the Workshop.
- **Watch memory.** About 110 KB is free after Bluetooth (BT costs about 96 KB). LVGL uses the system heap (`CONFIG_LV_USE_CLIB_MALLOC`); the Focus screen with its confirm dialog needs about 40 KB.
- **Content-sized flex rows must start-align.** `LV_FLEX_ALIGN_END` on an `LV_SIZE_CONTENT` row pushes the first child outside its box and clips it.
- **Leave `CONFIG_XPT2046_CONVERT_ADC_TO_COORDS` off.** `touch.c` errors at compile time if it's on, because every tap would land at (0,0).
- **The fonts have no glyphs outside their ranges.** Add new symbols with lv_font_conv; the tool runs in the scratchpad (`npm install --prefix`, never into `C:\Users\imibr`, where a `package.json` exists).

## History of this session (2026-10-23 → 10-24)

Cube OS was imported into this repo, then rebuilt milestone by milestone. The user wants **one milestone at a time**: plan it, build it, compile-check it, user flashes and tests it, then commit and push. Commit only when tested, or say clearly that it isn't.

| Commit | What |
|---|---|
| `cc091f1` M0 | Renamed to Pomo, moved to IDF v6.0.2 (`SPI2_HOST`/`SPI3_HOST`), cJSON dependency, `sdkconfig.defaults` (Classic BT only, coexistence, 1.5 MB single-app partition). |
| `18bb658` M1 | Landscape rotation, LVGL lock in `app_main`, PWM backlight, `board_pins.h`, heap logging, touch test screen (`POMO_TOUCH_TEST`). |
| `2a1109f` M2 | Phone UI removed; encoder HAL; Navigator with groups, back history and hold handler. |
| `6cac878` M3 | Fonts, theme, pet renderer, Home. |
| `9d045ac` M4 | Session engine and rules, all 9 screens, rules tests. |
| `8966a90`, `095798e` | XtraCube: new pins for test board one. |
| `49c637a` | Fixes found by the preview: the pet scaled-canvas transform ran LVGL out of memory (now rect-drawn), LVGL moved to the system heap, Wind/Workshop layout bugs, the 25/5 break reset, the shared SPI bus. |
| `1b382cb` | PC screen preview tool. |
| `85a8310` | Touch fix (driver option double-converted coordinates). |
| `df71b4a` M5 | JSON laptop link over USB. Tested on hardware. |
| `0152963` M8 | Bluetooth Classic SPP, NVS init. Tested on hardware both ways. |
| `283a840` | **Anthony Thillainathan** (teammate) "Milestone 9: protocol schema v2": camera drift from ML-Server-Pomo focus ticks (separate from laptop drift, `drift_kind`), `end` with scope `break`/`session`, `knob` message removed, UTF-8-safe names, fw 0.9, more tests. |
| `e40860d` M6 | Saved progress and settings in NVS. Tested: survives reset and power cycle. |
| `4932271` | LVGL task stack raised to 16 KB (the Workshop crashed); `[stack]` boot log. |
| `5daebea` A | Touch and knob parity: ◂ Back, −/+ buttons, grab rows, Focus Peek/End session buttons, ◂ − glyphs. Tested on hardware. |
| `9741998` B | New menu structure: Home tiles, Study screen (replaces Wind, Menu, Timer settings), Pet hub, Status, Wardrobe, Playground placeholder, session log in NVS. Tested on hardware. |
| C | Break -> next session: `READY` phase and Ready screen, Start now / End set on Intermission, petting hearts, protocol phase `"ready"`. Plus test timing (30 s phases) behind a Study screen chip. |
| PR #2 | Sidak: study length 15-90 in steps of 5, long rest = max(15, longest break), `earned` in `state`. Tested on hardware. |
| `linked-play` (M7) | Study circle over ESP-NOW, by invite (see "Linked play" below). Tested on two boards. |
| `frontend-parity` (merged to `main`) | Cross-referenced with Pomo-App `main` (`09fdd80`): heartbeat pause, pet.js's `sad` mood, session score on Intermission ("88% focused, +66 XP. Mochi is most proud."), level-up and ended-early notices (`theme::notice`), "Laptop" lamp on Home, Roman session numbers ("Session II of IV"), drift wording per kind ("Gone Idle", camera labels as the dashboard's), "+N XP so far" on Peek. Preview, soak and IDF-flag compile only; **not flashed**. Focus now peaks at ~50 KB of LVGL memory (was ~45). |
| Buzzer | Haptic-style clicks and timer chimes (`hal/buzzer.*`). Then louder clicks and longer, louder alarms, tuned by the user. |
| `f9154b1` | XtraCube: CFG B (touch flip) now applies to board 2 too. |
| `8c93a56` D | Playground (wind the key, the dance, feeding gears, sleep), petting everywhere (tap and stroke). Not confirmed on hardware. |
| E | Gears and hunger: studying earns gears, feeding spends them, the pet goes full → content → peckish → hungry. Gears bar on Home and the Playground, the `hungry` face, the `pet` message. Not yet flashed. |

Other notes:
- A screenshot gallery of every screen was published as a private artifact, "Pomo Screen Preview".
- Teammates push to `main` too. Always `git fetch` before pushing; rebase local unpushed commits onto `origin/main`, re-run tests, preview and build, then push.

## Linked play (M7, done)

- **ESP-NOW study circle, by invite. Tested on two boards; not yet tested with the laptop on Bluetooth at the same time, or a board leaving range mid-session.**
  - `app/circle_core.*` (pure, `test/circle_test.cpp`) and `comms/circle.*`. ESP-NOW broadcast on channel 1, no router, no esp_netif. Wire format v2: beacon `{name, phase, index, focused, pct, seq, group}` every 1.5 s (and on change), plus INVITE and DECLINE. A peer drops after 6 s silence.
  - By invite (user, 2026-10-24): the host invites up to 2 free friends from the Circle screen with its lengths; the guest's Invite screen has Accept / Decline; the session starts on all once every invite is answered (or the host presses Start). The group stays in step for the whole set (a member in BREAK/READY follows another starting the next session); ending the set leaves it.
  - The multiplier counts only group members who are studying; only their drift pauses it. Nearby friends who aren't in the group add nothing.
  - `-DPOMO_FAKE_PEER=1`: Ava accepts invites, Dev declines and invites this device.
  - Inviting from the Circle screen opens the Study screen first; its Start reads "Invite Sidak" and sends the invite with the lengths chosen there. A second invite while hosting uses the group's lengths.
  - Two test boards: COM5 (Ibrahim, pet Doug) and COM9 (Sidak, pet Mochi); names set over USB with a `config` line.
  - Frontend follow-up: the `circle` message doesn't say who is in this device's group yet.

## Unfinished from the original brief

- **Polish (the original "M9"; Anthony used that number for protocol v2):**
  - battery gauge behind `HAS_BATTERY` (ADC, smoothed, shown on Home)
  - dim the backlight during focus (not while peeking or drifting)
- **Frontend work (other repo):**
  - accept `"test": true` in the device's `start` message (test timing: 30 s phases instead of the `study`/`brk` minutes)
  - add `"ready"` to the `state` message's `phase` in `schema/pomo-device.schema.json` (milestone C: the break ran out and the device waits; a `start` or `end {scope: "break"}` starts the next session, `end {scope: "session"}` finishes)
  - send `time`/`tz` in `config`
  - follow the device's `start`/`state`/`end` instead of running its own timer and XP
  - drop hearts
- **Smaller loose ends:**
  - Touch calibration (`raw_x_min` and friends in `touch.c`) is still the old Cube OS panel's. Tap logs (`touch: tap raw ...`) give the values for this panel.
  - IM Fell's old-style figures make "Lv 1" read like "Lv i". Consider Plex for numbers.

## Next: the UI redesign (user's fix list)

The user tested the device and asked for these. Decisions already made with the user:
- **one Study screen** for everything timer-related
- **Home + 3 hubs**
- **wait and ask** after a break
- **Playground** = petting and reactions, plus feeding gears

Do them in order, one milestone at a time, checking each in the preview first.

- **A: touch and knob parity, navigation** (fix list #1, #2, #4, #5). **Done.**
  - **Parity:** everything the knob does can be done by tapping, and the reverse.
  - **Back:** an on-screen ◂ Back on every screen except Home and Focus.
  - **Tap targets:** bigger hit areas (`ext_click_area`).
  - **Wind fix:**
    - The knob moves between Study, Break and Start; a press grabs or releases a row.
    - Stylus gets −/+ buttons, and a tap only selects a row.
    - Today a press, or a tap, on Study jumps to Break and there's no way back.
  - **Focus:** visible **Peek** and **End session** buttons, keeping the confirm dialog. Touch is no longer locked during focus, a deliberate change from the brief.
  - **Fonts:** add the ◂ − + glyphs.
- **B: new menu structure** (#6, #8, #9, #11, #12). **Done.** On Home and the Pet hub a tap on the pet is a placeholder reaction (love face, wiggle) until D.
  - **Home:** the pet (tap to pet), a big Start, and tiles for **Study**, **Pet** and **Circle**.
  - **Study:** study/break lengths, the always-show-timer toggle, the session log and Start. It replaces Wind, Menu and Timer settings.
  - **Pet hub:**
    - **Status:** name, level and XP bar, mood, sessions completed, next accessory unlock.
    - **Playground:** milestone D.
    - **Wardrobe:** body/face/finish. It replaces "Mochi's workshop", because the pet's name varies.
  - **Session log** saved in NVS: sessions completed, sets finished, total focused minutes, today's sessions.
- **C: break → next session** (#10). **Done.** Holding the knob does nothing on Intermission and Ready, and the knob no longer winds the pet there (winding is D's Playground).
  - No auto-start after a break: "Ready for session N of 4?" with **Start next** / **Finish for now**.
  - **Start now** / **End set** buttons during the break.
  - Build on Anthony's end scopes (`endBreak`/`endSet`) and update the rules tests.
  - **Petting hearts** (user, 2026-10-24): tapping the pet shows little hearts floating up, like the frontend does. Decoration only: no hearts stat.
- **D: Playground and pet interaction** (#3, #7). **Built, not yet flashed.** The knob on the Playground winds (any detent) and feeds (press); a 2 s hold goes back, so Back isn't a knob stop there. Every body wears a key in the Playground. 12 clicks wind it fully: a 3 s dance (spins via squash and mirror, hops, hearts, tune), then it unwinds; left alone 3 s it runs down, and after 20 s it falls asleep. One gear in flight at a time, unlimited.
  - **Petting:** tap or drag the pet anywhere it appears to get a reaction (mood change, bounce).
  - **Winding:** in the Playground the knob winds its key and it spins.
  - **Feed it gears:** a little catch animation.
  - No hearts stat or other stats from play (the petting hearts are decoration, from C).
