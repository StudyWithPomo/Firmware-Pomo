# Pomo

Firmware for **Study Buddy**: a steampunk clockwork pet on your desk that grows when you focus. It runs a hidden Pomodoro timer, owns your XP and level, and reacts when the laptop dashboard ([Frontend-Pomo](https://github.com/StudyWithPomo/Frontend-Pomo)) reports a drift.

Built on [Cube OS v1 alpha](https://github.com/ibbydiggy250/Cube_OS_v1_alpha) (ESP-IDF + LVGL 9). Work in progress: the full screen flow and session timer run on the device; the laptop link, saved progress, study circle and Bluetooth come next.

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

## Laptop protocol

JSON lines at 115200 baud over USB serial (and later Bluetooth SPP). The message set is documented in the frontend repo's README and will be summarized here once the link is implemented.
