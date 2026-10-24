#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "lvgl.h"

// Steampunk type from the design, at half the canvas sizes (generated, see fonts/README.md)

// IM Fell English SC: titles, buttons, labels
extern const lv_font_t font_fell_12;
extern const lv_font_t font_fell_14;
extern const lv_font_t font_fell_16;
extern const lv_font_t font_fell_25;

// Libre Caslon Text: body copy (speech bubbles, friend names)
extern const lv_font_t font_caslon_12;

// IBM Plex Mono Regular: small numbers ("25 / 5", "Lv 3 · 340 XP")
extern const lv_font_t font_plex_12;

// IBM Plex Mono Light, digits, colon and space only: nixie tubes and timers
extern const lv_font_t font_plex_22;
extern const lv_font_t font_plex_28;
extern const lv_font_t font_plex_50;

// Symbols merged into the Fell 12/14/16 and Caslon fonts from DejaVu Sans
#define UI_POINTER "\xE2\x96\xB8" // ▸ U+25B8, the focus pointer (also in Fell 25)
#define UI_BACK    "\xE2\x97\x82" // ◂ U+25C2
#define UI_MINUS   "\xE2\x88\x92" // − U+2212

#ifdef __cplusplus
}
#endif
