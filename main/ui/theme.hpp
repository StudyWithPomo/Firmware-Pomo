#pragma once
#include <stddef.h>
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

// For screens where turning always changes a value (Wind, Intermission): puts the group in
// edit mode and keeps it there, since LVGL leaves edit mode whenever focus moves
void always_editing(lv_group_t *group);

// A hidden "▸ " label for focusable()
lv_obj_t *pointer(lv_obj_t *parent, const lv_font_t *font);

// Outlined button: [▸] text ........ detail. Focusable; returns the button.
lv_obj_t *button(lv_obj_t *parent, lv_group_t *group, const char *text, const lv_font_t *font,
                 const char *detail = nullptr, const lv_font_t *detail_font = nullptr);

// Outline-only focus (the Workshop's rows): highlight outline, labels turn BRASS_HI
void focusable_outline(lv_obj_t *obj, lv_group_t *group, lv_obj_t *pointer = nullptr);

// Both kinds of knob stop show a verdigris outline while grabbed (the group is editing).

// Did the input being handled come from the knob? False for touch. Events sent with no input
// device (the PC preview) count as the knob.
bool from_knob();

// ---- Touch ----

// Extra hit area around small touch targets, beyond what's drawn
constexpr int TAP_MARGIN = 6;

// Title row: "◂ Back  Title ........" (space-between, full width). Add right-hand items to
// `row`. Back is not in the group yet: add it last with lv_group_add_obj, so it's the knob's
// last stop. Hold still goes back too.
struct Header {
    lv_obj_t *row;
    lv_obj_t *back;
    lv_obj_t *title;
};
Header header(lv_obj_t *content, const char *title, const lv_font_t *font = &font_fell_16);

// "− +" buttons for touch, in a row added to `parent`. A press (and holding) sends
// LV_KEY_LEFT / LV_KEY_RIGHT to `target`, just like turning the knob while it's grabbed,
// and moves the knob to `target`. Not knob stops.
lv_obj_t *step_buttons(lv_obj_t *parent, lv_obj_t *target);

// Menu row: 32 px tall with a divider under it, "[▸] text ..... detail". Focusable.
// Returns the row; the detail label (if any) is child 1.
lv_obj_t *list_row(lv_obj_t *parent, lv_group_t *group, const char *text,
                   const char *detail = nullptr, const lv_font_t *detail_font = nullptr,
                   uint32_t detail_color = BRASS);

// Make a pet pettable, the same on every screen it appears on (not Focus, where a tap hushes
// it): `pet` is a pet_view sitting in `porthole`, and its hit area grows to the porthole.
//   tap:    the pet is pleased (AppState::petted), hearts and a hop (pet_hearts)
//   stroke: dragging across it adds a squish, a heart and a purr every ~30 px (pet_stroke)
// `can_pet`, if given, is asked first (the Playground says no while the pet dances).
// Decoration only: there's no hearts stat, and petting earns nothing.
void pettable(lv_obj_t *pet, lv_obj_t *porthole, bool (*can_pet)() = nullptr);

// `porthole` hops and little pixel hearts float up out of it and fade: three, or fewer the
// hungrier the pet is (2 peckish, 1 hungry). The hearts live on the top layer and delete
// themselves, so they outlast a screen change.
void pet_hearts(lv_obj_t *porthole);

// One stroke: the pet squishes wider for a moment, one heart rises (not when hungry) and it
// purrs
void pet_stroke(lv_obj_t *pet, lv_obj_t *porthole);

// The gears bar: a gear icon and one pip per gear the pet can hold, filled for each held.
// Follows AppState's gears.
lv_obj_t *gears_bar(lv_obj_t *parent);

// The pet squishes wider and back, for a stroke or a gear caught (pet_view_set_squash)
void pet_squish(lv_obj_t *pet);

// A notice card at the top of the screen for `ms` ("Mochi reached Level 5!" / "Goggles
// unlocked."). It lives on the top layer, so it stays over a screen change; a tap dismisses it
// and a new notice replaces it. `detail` may be empty.
void notice(const char *title, const char *detail, uint32_t ms = 4000);

// A hold handler that does nothing, for screens where going back makes no sense (a break)
void ignore_hold();

// ---- Nixie tubes ----

struct NixieSpec {
    int tube_w, tube_h;
    int colon_w;
    int radius;
    int gap;
    const lv_font_t *font;  // digits font (font_plex_22/28/50)
};

// MM:SS in four tubes and a colon. Unlit rows have empty tubes.
lv_obj_t *nixie_row(lv_obj_t *parent, const NixieSpec &spec, bool lit);
void nixie_set(lv_obj_t *row, int seconds);

// "MM:SS"
void format_mmss(char *buf, size_t size, int seconds);
// Session numbers the way the dashboard writes them: "I" to "IV" ("?" outside 1-4)
const char *roman(int n);

// ---- Study circle lamps ----

// "● Ava · focused" (verdigris lamp) or "● Dev · drifting" (copper lamp)
lv_obj_t *lamp(lv_obj_t *parent, const char *name, bool focused);
// Refill `container` with a lamp per friend in this device's study group, or `empty_text`
// when studying alone
void fill_lamps(lv_obj_t *container, const char *empty_text = "Studying solo");

} // namespace theme
