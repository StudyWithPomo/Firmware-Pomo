#pragma once
#include <stdint.h>

#include "lvgl.h"
#include "fonts.h"

// Steampunk look from the "Device screens · steampunk" design canvas, at half its size
// (the canvas is drawn at 2x for a 320x240 screen).
namespace theme {

// ---- Colour tokens ----
constexpr uint32_t BRASS        = 0xC9A15A; // lines, text, focused fill
constexpr uint32_t BRASS_HI     = 0xF4DDA0; // focus outline, highlighted text
constexpr uint32_t BRASS_LIGHT  = 0xE8CF94; // big numbers on cards
constexpr uint32_t PANEL        = 0x1E1510; // screen background, text on brass
constexpr uint32_t PORTHOLE     = 0x2B1E16; // porthole glass
constexpr uint32_t PARCHMENT    = 0xEFE3C8; // speech bubbles, light body text
constexpr uint32_t INK          = 0x2A1A10; // text on parchment
constexpr uint32_t COPPER       = 0xB5462A; // drift alarm band
constexpr uint32_t ALARM_TEXT   = 0xF6ECD6;
constexpr uint32_t PAUSED       = 0xE8A48F; // "2x paused"
constexpr uint32_t VERDIGRIS    = 0x6FAE9E; // intermission frame
constexpr uint32_t VERDIGRIS_HI = 0x8FD1BF; // focused lamps, break digits
constexpr uint32_t NIXIE        = 0xFFC08A; // lit nixie digits
constexpr uint32_t NIXIE_GLOW   = 0xFF5A00;
constexpr uint32_t TUBE_TOP     = 0x3A2416; // unlit tube gradient
constexpr uint32_t TUBE_BOTTOM  = 0x140D09;
constexpr uint32_t TUBE_RIM     = 0x6B5233; // tube borders, row dividers
constexpr uint32_t MUTED        = 0xB8A27C; // hint lines

inline lv_color_t c(uint32_t hex) { return lv_color_hex(hex); }

// Call once at startup, with the LVGL lock held
void init();

// ---- Builders ----

// Fill `parent` with the panel and a double brass border; returns the padded content box
// (a flex column with space-between, like every screen in the design).
lv_obj_t *frame(lv_obj_t *parent, uint32_t border_color = BRASS);

// A transparent container with no padding, border or scrolling
lv_obj_t *box(lv_obj_t *parent);
// A transparent flex row; `main` is the main-axis alignment
lv_obj_t *row(lv_obj_t *parent, lv_flex_align_t main = LV_FLEX_ALIGN_SPACE_BETWEEN, int gap = 0);
// A transparent flex column
lv_obj_t *column(lv_obj_t *parent, int gap = 0);

lv_obj_t *label(lv_obj_t *parent, const char *text, const lv_font_t *font, uint32_t color = BRASS);

// Round brass-rimmed porthole of `size` px, centred content
lv_obj_t *porthole(lv_obj_t *parent, int size, int rim = 3);

// Parchment speech bubble
lv_obj_t *bubble(lv_obj_t *parent, const char *text, int width);

// Outlined chip ("Menu", "Pomodoro"); `filled` for a chosen option
lv_obj_t *chip(lv_obj_t *parent, const char *text, const lv_font_t *font, bool filled = false);

// ---- Focus ----

// Make `obj` a knob stop: adds it to `group` in call order and applies the focus look
// (filled brass, highlight outline, every label inside turns PANEL). If `pointer` is
// given it is shown only while focused (the design's ▸).
void focusable(lv_obj_t *obj, lv_group_t *group, lv_obj_t *pointer = nullptr);

// A hidden "▸ " label for focusable()
lv_obj_t *pointer(lv_obj_t *parent, const lv_font_t *font);

// Outlined button: [▸] text ........ detail. Focusable; returns the button.
lv_obj_t *button(lv_obj_t *parent, lv_group_t *group, const char *text, const lv_font_t *font,
                 const char *detail = nullptr, const lv_font_t *detail_font = nullptr);

} // namespace theme
