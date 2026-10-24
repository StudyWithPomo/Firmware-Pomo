# Pomo

Firmware for **Study Buddy**: a steampunk clockwork pet on your desk that grows when you focus. It runs a hidden Pomodoro timer, owns your XP and level, and reacts when the laptop dashboard ([Frontend-Pomo](https://github.com/StudyWithPomo/Frontend-Pomo)) reports a drift.

Built on [Cube OS v1 alpha](https://github.com/ibbydiggy250/Cube_OS_v1_alpha) (ESP-IDF + LVGL 9). Work in progress: the screens, session timer, laptop link (USB and Bluetooth) and saved progress run on the device; the study circle and the buzzer/battery come next.

XP, the pet, your name and the timer settings are kept in flash (NVS namespace `pomo`) and restored at boot. Settings are written about 2 s after they stop changing, and XP as soon as a session is scored. To start over, erase the flash with `idf.py -p PORT erase-flash` (this also forgets Bluetooth pairings).

## Hardware

| Part | Detail |
|---|---|
| MCU | Original ESP32 (Classic BT + Wi-Fi) |
| Display | 2.8" ILI9341, used landscape 320×240 |
| Touch | XPT2046 resistive, separate SPI bus |
| Input | Rotary encoder with push button |
| Extras | Passive buzzer; battery gauge on Device A |

### Pin map

| Signal | GPIO |
|---|---|
| LCD MOSI / CLK / CS / DC / RST / BL | 14 / 27 / 13 / 12 / 2 / 26 |
| Touch MOSI / MISO / CLK / CS / IRQ | 21 / 23 / 33 / 32 / 22 |
| Encoder, buzzer, battery | Not assigned yet |

GPIO 12 (LCD DC) is a strapping pin. If boot is flaky, move DC to a free pin.

## Build and flash

Requires **ESP-IDF v6.0.2**.

```bash
idf.py set-target esp32
idf.py build
idf.py -p COM4 flash monitor
```

`sdkconfig` is generated from `sdkconfig.defaults` and isn't committed. Delete `sdkconfig` after changing the defaults so they take effect. The defaults turn on Classic BT only (no BLE), Wi-Fi/BT coexistence, and the 1.5 MB single-app partition table.

## Session rules test

The timer and XP rules (`main/app/session_core.cpp`) have no ESP-IDF dependencies, so they can be tested on a PC with any C++17 compiler:

```bash
g++ -std=c++17 -Wall -I main test/session_test.cpp main/app/session_core.cpp -o session_test && ./session_test
```

The study circle rules (`main/app/circle_core.cpp`: the beacon format, friends joining and leaving) are tested the same way:

```bash
g++ -std=c++17 -Wall -I main test/circle_test.cpp main/app/circle_core.cpp -o circle_test && ./circle_test
```

## Study circle (linked play)

Pomo devices near each other form a study circle: each friend nearby adds 1× XP, studying or resting (up to 3×), and the bonus pauses while anyone studying drifts. Every device joins automatically. Work in progress on the `linked-play` branch: the circle logic runs, and the ESP-NOW radio comes next.

To try it on one board, build with scripted friends:

```bash
idf.py -DPOMO_FAKE_PEER=1 build
```

Dev is always nearby but resting, and Ava studies for 100 s (drifting from 40 to 55 s), goes quiet for 20 s, and repeats. Build with `-DPOMO_FAKE_PEER=0` to turn them off again.

## Screen preview

`tools/preview` renders every screen to 320×240 PNGs on a PC, without hardware. It compiles the real UI code and session engine against LVGL with the device's own configuration, using small stand-ins for ESP-IDF. It also prints how much LVGL memory each screen uses. Run `idf.py build` once first (it fetches LVGL and generates the config), then, with a MinGW-w64 or Linux g++ on the path:

```bash
tools/preview/build.sh
```

The PNGs land in `tools/preview/out/`.

## Laptop protocol

One JSON object per line, over the USB serial port (115200 baud) or Bluetooth. Both work at the same time and carry the same messages. The full contract is [`schema/pomo-device.schema.json`](https://github.com/StudyWithPomo/Frontend-Pomo/blob/main/schema/pomo-device.schema.json) in Frontend-Pomo.

**Bluetooth:** the device is discoverable as **Pomo-XXXX** (Bluetooth Classic SPP, the last digits come from its address). Pair it in the laptop's Bluetooth settings; there's no PIN (if one is asked for, it's 1234). Windows then adds a "Standard Serial over Bluetooth link" COM port; open that one (the outgoing port) from the dashboard like the USB port. The device is the source of truth for the timer, drift, XP and level; the laptop reports what it sees (sites, idle, camera focus) and settings. Lines that aren't JSON, and unknown messages, are ignored, so the device's own log lines on the same port are harmless.

**Laptop → device**

```json
{"t":"drift","active":true,"label":"YouTube","kind":"site"}
{"t":"drift","active":false}
{"t":"config","user":"Sidak","pet":{"name":"Mochi","body":"bot","face":"plain","color":"brass"},"time":1792790040,"tz":-240}
{"t":"start","study":25,"brk":5}
{"t":"peek"}
{"t":"end","scope":"break"}
{"t":"end","scope":"session"}
{"t":"focus","ts":1792790040.12,"focus":42,"state":"unfocused","reason":"looking_away","window":"neutral","yaw":21.3,"pitch":-3.1,"ear":0.27}
```

- `config`: `time` (epoch seconds) and `tz` (minutes east of UTC, i.e. `-new Date().getTimezoneOffset()`) are optional and set the device clock. `level`, `xp` and `hearts` are ignored.
- `start` begins a session only if none is running; lengths snap to the allowed values.
- `drift` is for what the laptop sees: `kind` is `site` or `idle`, and `label` is required when `active` is true.
- `end` with `scope: "break"` ends a break (the next session starts) or the long rest (back to idle). `scope: "session"` ends the whole set: mid-study the session is scored first and an `end` comes back with `early: true`.
- `focus` is a tick from [ML-Server-Pomo](https://github.com/StudyWithPomo/ML-Server-Pomo) (its stdout line with `"t":"focus"` added by the laptop). The device decides camera drift, during study only: it starts after 10 s of `unfocused` or `drifting`, and ends after 3 s of `focused` or 5 s without a tick. `calibrating` changes nothing. The label comes from `reason` (Away, Drowsy, Looking away, Slouching, Off task, or Camera).
- `break`, `finished`, and `end` without a `scope` are ignored: the device runs the session.

**Device → laptop**

```json
{"t":"hello","fw":"0.9"}
{"t":"start","study":25,"brk":5,"session":1}
{"t":"state","left":522,"total":1500,"phase":"study","focus":"focused","session":1,"drift":"","drift_kind":"","drift_s":0,"mult":2,"xp":340,"level":1}
{"t":"end","pct":90,"xp":44,"level":3,"total_xp":1044,"focused_min":22,"elapsed_min":25,"penalty":false,"early":false}
{"t":"circle","friends":[{"name":"Ava","focused":true,"pct":0}]}
```

- `state` is sent every second during a session or break (`phase`: `study`, `break`, `long_rest`), and once when it goes back to `idle`.
- `state.drift_kind` says where the drift shown comes from: `site`, `idle`, `camera`, or `""` when focused. The laptop's drift is shown when both are active.
- `start` is sent for every session, including ones started on the device and the next session after a break.
- `end.xp` is the XP change from that session (negative after the under-50% penalty); `total_xp` is the new total.
- Raw knob and touch input is never sent.

**Changes the frontend needs:** send `time`/`tz` in `config`; treat `start`, `state` and `end` from the device as the session (instead of running its own timer); drop hearts and the `knob` handler; send `end` with a `scope`; forward ML-Server-Pomo lines as `focus` instead of sending camera drifts. With no laptop connected the device runs sessions alone and counts every minute as focused.
