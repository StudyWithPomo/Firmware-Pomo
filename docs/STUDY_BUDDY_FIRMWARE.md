# Study Buddy Firmware: Build Brief for Claude Code

You are turning **Cube OS v1 alpha** (`ibbydiggy250/Cube_OS_v1_alpha`) into the firmware for **Study Buddy**, a hackathon project (SBU Hacks). Read this whole file before changing code. When this file and the existing code disagree, this file wins. When this file and `FEATURES.md` disagree, this file and `SCOPE.md` win (the main conflict: **hearts were cut**).

---

## 1. What we're building, and why

**Pitch:** A steampunk clockwork pet on your desk that grows when you focus. It never blocks anything. It notices when you drift, names the drift ("YouTube 0:31"), and gives you a reason to come back.

**The system:**
- **Device (this firmware):** ESP32 + 2.8" touchscreen + rotary encoder + buzzer + lights. Hosts the pet, runs the study timer, and owns all progress.
- **Laptop dashboard** (already built, separate repo `StudyWithPomo/Frontend-Pomo`): sets up sessions, detects distractions, and shows stats and a "time card" at the end. It talks to the device over USB serial or Bluetooth.
- **Two devices** at the demo. They find each other over ESP-NOW and form a **study circle** for an XP multiplier.

**The device is the source of truth** for the timer, focus state, XP, and level. The laptop only *reports drift events* and *displays what the device sends*. The laptop must be optional: the device runs full sessions alone, just without drift detection.

### Why Cube OS
Cube OS is already ESP-IDF 5.5 + **LVGL 9.4** + `esp_lvgl_port` 2.7, with working ILI9341 display and XPT2046 touch drivers, plus a screen Navigator and a reactive AppState (`lv_subject`). We chose it over VoidPhone (LovyanGFX) because:
- LVGL has **native rotary-encoder support** (`LV_INDEV_TYPE_ENCODER` + `lv_group`). The encoder is the primary control, so menus come almost free.
- We need 9 screens with buttons, lists, arcs, and bars. LVGL provides these widgets; LovyanGFX would mean hand-building every one.
- The Navigator/AppState pattern already matches our screen flow.

The trade-off is that LVGL uses more RAM, and we will also run Bluetooth Classic and Wi-Fi (ESP-NOW). **Keep RAM usage in mind for every change.**

---

## 2. Hardware

| Part | Detail |
|---|---|
| MCU | **Original ESP32** (Classic BT + BLE + Wi-Fi). Not C3/S3 |
| Display | 2.8" ILI9341, 240×320 native, used **landscape 320×240** |
| Touch | XPT2046 resistive, separate SPI bus |
| Encoder | Rotary encoder with push button |
| Buzzer | Passive buzzer (PWM tones) |
| Lights | Status lamp(s): focused / drift / break |
| Power | Device A: battery (needs a battery gauge). Device B: USB |

### Current pin map (from the repo, do not change without reason)
| Signal | GPIO |
|---|---|
| LCD MOSI / CLK / CS / DC / RST / BL | 14 / 27 / 13 / 12 / 2 / 26 (HSPI) |
| Touch MOSI / MISO / CLK / CS / IRQ | 21 / 23 / 33 / 32 / 22 (VSPI) |

### Pins not yet assigned (put them in one `board_pins.h`, marked TODO-confirm)
Suggested free pins. **Ask the user to confirm the wiring before relying on these:**
- Encoder A / B / button: 16 / 17 / 4 (internal pull-ups on)
- Buzzer: 25 (LEDC PWM)
- Status lamp: 5, or an RGB LED on 18/19
- Battery ADC (Device A): 34 or 35 (input-only, through a voltage divider)

### Known hardware risk
**GPIO 12 (LCD DC) is a strapping pin.** If it's pulled high at boot, the ESP32 sets flash to 1.8 V and fails to boot. If boot is flaky, move DC to a free pin. Don't change it pre-emptively.

---

## 3. Required changes to the Cube OS base

Do these first, in order. Each must work before moving on.

### 3.1 Rotate the display to landscape
**What:** In `main/hal/display.c`, after `lvgl_port_add_disp()`:
```c
display_handle = lvgl_port_add_disp(&disp_cfg);
if (display_handle == NULL) return ESP_FAIL;

lvgl_port_lock(0);
lv_display_set_rotation(display_handle, LV_DISPLAY_ROTATION_90);
lvgl_port_unlock();
```
- Keep `hres = 240`, `vres = 320` (the panel's native size). The `.rotation` struct must stay equal to the initial esp_lcd settings.
- Keep hardware rotation (`flags.sw_rotate = false`, the default). It's faster and needs no extra buffer.
- If the image is upside down, use `LV_DISPLAY_ROTATION_270`.

**Why:** The product is landscape (320×240). esp_lvgl_port handles hardware rotation through esp_lcd swap/mirror, so this is one call rather than rewriting the driver.

### 3.2 Verify touch after rotation
**What:** Build a temporary test screen that draws a dot where you tap. Tap all four corners.
- LVGL 9 rotates pointer input to match the display's rotation, so the **portrait calibration in `touch.c` should stay as is**.
- If the axes are swapped or mirrored, fix `touch_cfg.flags` (`swap_xy`, `mirror_x`, `mirror_y`). **Don't** change the calibration math, and don't rotate touch in two places.

**Why:** Touch is used for direct selection and for petting the pet. Getting the axes wrong is the most common rotation bug.

### 3.3 Fix the LVGL thread-safety race in `main.cpp`
**What:** `ui_root_init()` and `Navigator::instance().navigate()` run from `app_main` while the esp_lvgl_port task is already running. Wrap them:
```c
lvgl_port_lock(0);
ui_root_init();
Navigator::instance().navigate(ScreenID::BOOT);
lvgl_port_unlock();
```
**Rule for the whole project:** LVGL is not thread-safe. Any LVGL call from outside the LVGL task (comms task, timer task, ESP-NOW callback) must hold `lvgl_port_lock()`. **Preferred pattern:** non-UI tasks only update AppState (§5). UI updates come from `lv_subject` observers or `lv_timer`, which already run inside the LVGL task.

**Why:** Without the lock, you get random crashes and corrupted frames that are very hard to debug at a hackathon.

### 3.4 Remove hardcoded resolution
**What:** Delete `DISP_WIDTH`/`DISP_HEIGHT` defines from `main.cpp`. Layout code should use `lv_display_get_horizontal_resolution(NULL)` / `..._vertical_resolution(NULL)`, or percentages and flex/grid layouts.

**Why:** After rotation the logical size is 320×240. Hardcoded 240×320 values break every layout.

### 3.5 Backlight to PWM
**What:** Replace the GPIO on/off backlight (GPIO 26) with LEDC PWM. Expose `display_set_brightness(uint8_t pct)`.

**Why:** The spec dims the backlight during focus (saves battery on Device A, and the pet stays calm).

### 3.6 Strip phone features
**What:** Remove the phone, SMS, contacts, settings, and lock screens; the home app grid; and cellular/Wi-Fi-settings placeholders in `SystemAPI`/`AppState`. **Keep** the HAL, Navigator, `ui_root` content area, the AppState/`lv_subject` pattern, and `SystemAPI::getCurrentTime/setCurrentTime`.

**Why:** Less code means less RAM and a clean slate for the Study Buddy screens.

### 3.7 Add the rotary encoder as an LVGL input
**What:** Create an `LV_INDEV_TYPE_ENCODER` indev, either with `lvgl_port_add_encoder()` (needs the `espressif/knob` and `espressif/button` components) or a custom read callback using the ESP-IDF PCNT peripheral. Then:
- Create **one `lv_group_t` per screen**. Add that screen's focusable widgets in knob order, and call `lv_indev_set_group(encoder, group)` when the screen loads.
- **Turn** moves focus or changes a value. **Press** selects.
- **Hold 2 s** = back / end session early. Detect it in the button driver and post an event; the default LVGL long-press timing is too short.

**Why:** The encoder is the main control on every screen (it's core, not a stretch goal).

### 3.8 Memory and config
- In menuconfig, set the BT controller mode to **Classic BT only** (not dual mode), unless BLE is needed. This frees RAM.
- Keep the LVGL draw buffer at about 40 rows, single buffer, DMA (as now).
- Enable only the Montserrat font sizes actually used.
- Check free heap after BT + Wi-Fi + LVGL init (`heap_caps_get_free_size`) and log it at boot.

---

## 4. What the LCD must support (screens)

Logical resolution is **320×240 landscape**. The style is steampunk: brass/copper/verdigris colors, nixie-tube digits, and a porthole frame around the pet. The **focused item is always filled brass with a ▸ pointer.** Menu rows are about 32 px tall.

Replace `ScreenID` with:

| ID | Screen | Contents | Controls |
|---|---|---|---|
| `BOOT` (00) | Boot | "Study Buddy", day and time, pet in porthole, greeting ("Good evening, {user}. {pet} missed you."), "Ready for a study session?", buttons **Yes, wind it** / **Not yet** | Knob picks a button |
| `HOME` (01) | Home | Pet name + level, **Menu** chip, pet in porthole, study circle card (multiplier + friend names), **Begin 25 / 5** button | Knob cycles Begin → Circle → Menu |
| `WIND` (02) | Wind the timer | "Pomodoro" tag, **Study** row with nixie digits, **Break** row showing the allowed range ("allowed 5–8"), hint line | Turn changes the value; press moves to the next row / starts |
| `FOCUS` (03) | Focus (timer hidden) | "In Session", XP multiplier badge, large pet, **four dark nixie tubes** + "Press to peek", friend status lamps ("Ava · focused") | Press = peek. Touch **locked** |
| `PEEK` (03b) | Peek | Same as Focus with the nixie timer lit and a progress rail. Returns to dark after **5 s** | Implement as a state of FOCUS, not a separate screen rebuild |
| `DRIFT` (04) | Drift | Copper alarm band naming the drift and its duration ("YouTube Detected 0:31"), annoyed pet, "Kindly return to your studies.", small timer, "2× paused" | Shown while a drift is active; returns to FOCUS when it ends |
| `INTERMISSION` (05) | Break | Verdigris frame, break countdown, wound-up pet, "Turn or hold to wind {pet}" | Touch **unlocked**: petting the pet or turning the knob animates winding its key |
| `MENU` (06) | Menu | Begin session (25/5), Study circle (N near), {pet}'s workshop, Timer settings | Knob list |
| `WORKSHOP` (07) | Pet customization | Live pet preview, rows for **Body** (Bot/Cat/Dog), **Face** (Plain/Monocle), **Finish** (4 swatches) | The outlined row is where the knob is; a filled chip is the current choice |

**No hearts anywhere.** FEATURES.md still mentions hearts on Intermission; ignore that.

### Navigation rules
- Navigator rebuilds the content area on `navigate()`. That's fine, but **delete the old screen's `lv_group` and observers** to avoid leaks.
- Hold 2 s = back from MENU/WORKSHOP/WIND. During FOCUS it ends the session early (confirm first).
- DRIFT and PEEK are overlays/states on the focus flow. Don't rebuild the whole screen every second.

---

## 5. Features and logic

### 5.1 AppState (rewrite)
Replace the phone subjects with `lv_subject`s for: `screen`, `timer_remaining_s`, `phase` (idle/study/break/long_rest), `session_index` (1–4), `focus_state` (focused/drift), `drift_label`, `drift_seconds`, `peek_active`, `always_show_timer`, `xp`, `level`, `multiplier`, `circle_friends`, `pet_body`, `pet_face`, `pet_finish`, `pet_mood`, `pet_name`, `link_connected`, `battery_pct`.

Screens observe the subjects and never read globals directly. Background tasks change state through AppState functions that take the LVGL lock (or post to a queue drained by an `lv_timer`).

### 5.2 Timer
- Study lengths: **15 / 25 / 45 / 60 min**. Default **25 / 5**.
- Break must be **⅕ to ⅓ of study time** (15 → 3–5, 25 → 5–8, 45 → 9–15, 60 → 12–20). The UI only offers allowed values.
- **4 sessions per set, then a 15-min long rest.**
- Timer is **hidden by default**. Peek shows it for 5 s, or the "always show" setting keeps it visible.
- **Buzzer chime** at the start of each break.
- Drive the timer from `esp_timer` or a FreeRTOS task, not from LVGL redraws, so it stays accurate when the UI is busy.

### 5.3 Focus tracking and XP rules (the device applies all of these)
| Rule | Value |
|---|---|
| XP | +1 per focused minute × circle multiplier |
| Level | 500 XP per level |
| Multiplier | 1 + friends in circle, capped at 3× (2× with our two devices) |
| Bonus pause | Multiplier drops to 1× while **anyone** in the circle is drifting |
| Drift minute | A minute counts as drift if 15+ s of it was drift |
| Penalty | Session under 50% focus: no XP, −10 XP, never below the start of the current level (no de-leveling) |
| Focus % | focused minutes ÷ elapsed minutes |

Persist `xp`, `level`, and pet settings in **NVS**. Save at session end and on settings change, not every second (flash wear).

**No hearts, no health, no pet death.**

### 5.4 The pet
- Source of truth: `pet.js` in the frontend repo. It's a **16×16 grid** where each layer is a list of `[x, y, w, h]` rectangles painted in one palette color. **Port the data to a C header** (`pet_data.h`) as const arrays.
- Combos: 3 bodies (bot / cat / dog) × 2 faces (plain / monocle) × 4 finishes (brass / copper / pewter / verdigris). Palettes swap colors; there is no image per combo.
- Layer order (match `drawPet()`): body + line → metal → body-specific (bot `key`, `cat`, or `dog`) → mood → goggles (Lv ≥ 5) → tie (Lv ≥ 10) → hat (Lv ≥ 15) → monocle (if face = monocle).
- Moods (visual only): **happy** (idle), **focus**, **annoyed** (drift), **wound up** (break; `pet.js` has `love`, use it unless the team says otherwise), **shh** (tapped during focus).
- **Rendering, RAM-light approach:** draw into a **16×16 RGB565 `lv_canvas`** (512 bytes), then upscale with `lv_image_set_scale()` and `lv_image_set_antialias(false)` for crisp pixels. Only fall back to a full-size canvas if scaling looks wrong. A 128×128 canvas costs 32 KB.
- Name: up to 10 letters, set from the laptop via `config`.

### 5.5 Laptop link (JSON lines)
- 115200 baud, one JSON object per line, UTF-8.
- **Listen on both USB serial (UART0) and Bluetooth Classic SPP at once.** Both feed **one** line parser and message handler. A paired SPP device appears to the laptop as a serial port, so the frontend's Web Serial code works for both.
- Read the current protocol in the frontend repo README, then apply these changes:
  - **Remove** `hearts` from all messages.
  - **Device → laptop:** `start` (session began, including device-initiated), `end` (focus %, XP earned, level), `state` (time left + focus state, sent about every second), `circle` (friends: `name`, `focused`, `pct`).
  - **Laptop → device:** `drift` (started/stopped + label, e.g. "YouTube"), `config` (pet settings + current time; set the RTC from it).
- Ignore malformed lines; never crash on bad input.

### 5.6 Study circle (ESP-NOW)
- Wi-Fi in STA mode, no router, ESP-NOW broadcast. Each device beacons `{name, focused, pct, in_session}` about every 1–2 s.
- Drop a peer if it hasn't been heard from in about 6 s.
- Feeds the multiplier, the bonus pause, friend lamps on FOCUS/HOME, and the `circle` message to the laptop.
- Must coexist with BT Classic (ESP32 supports Wi-Fi/BT coexistence; enable it in menuconfig).
- Build this in parallel using dummy peer data.

### 5.7 Lights, buzzer, power
- **Status lamp:** focused / drift / break colors or patterns.
- **Buzzer:** LEDC tone. Chime at break start, optional short tick on knob press.
- **Backlight:** dim during focus (except while peeking or drifting), full brightness elsewhere.
- **Battery gauge (Device A):** ADC through a divider, smoothed, shown on HOME. Make this a compile-time flag (`HAS_BATTERY`), because both devices run the same firmware.

### 5.8 Standalone mode
With no laptop connected, everything works except drift detection. Sessions run, XP accrues (every minute counts as focused), and the circle still works.

---

## 6. Build order (stop and test after each step)

1. **Base fixes:** §3.1–3.5. Rotated screen, verified touch, no races, PWM backlight.
2. **Strip phone UI + encoder:** §3.6–3.7, with a test screen whose buttons are navigable by knob.
3. **Pet renderer + HOME and FOCUS screens.**
4. **Timer + full screen flow** 00 → 07, including PEEK and DRIFT states.
5. **JSON protocol over USB serial.**
6. **NVS progress** (XP, level, pet settings).
7. **ESP-NOW study circle** (can start in parallel with dummy data).
8. **Bluetooth SPP** on the same message handler.
9. **Touch petting, Workshop, lights, buzzer, polish.**

Log free heap after steps 1, 7, and 8.

---

## 7. Out of scope (don't build)
Camera, audio/voice, phone dock, PIR/ultrasonic sensors, night light, daily summary, hearts/health/pet death.

## 8. Working rules for Claude Code
- Ask before changing pins, the partition table, or the LVGL/IDF versions.
- Keep the HAL in C and the app/UI in C++, matching the repo's style.
- One screen per file in `main/ui/screens/`.
- If a spec detail is ambiguous, check `SCOPE.md`, then `FEATURES.md`, then ask. Don't invent behavior.
