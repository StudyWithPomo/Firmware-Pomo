#pragma once
// Pet sprite, ported from pet.js in StudyWithPomo/Frontend-Pomo (the source of truth).
// A 16x16 grid. Every layer is a list of paints; each paint fills [x, y, w, h] rectangles
// with one palette slot. Paints keep pet.js's key order, because later ones draw on top.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { uint8_t x, y, w, h; } PetRect;

typedef enum {
    PET_SLOT_BODY,
    PET_SLOT_LINE,
    PET_SLOT_CHEEK,
    PET_SLOT_SHADE,
    PET_SLOT_HI,
    PET_SLOT_RIVET,
    PET_SLOT_MONO,
    PET_SLOT_COUNT
} PetSlot;

typedef struct { uint8_t slot; const PetRect *rects; uint8_t count; } PetPaint;
typedef struct { const PetPaint *paints; uint8_t count; } PetLayer;

typedef enum { PET_BODY_BOT, PET_BODY_CAT, PET_BODY_DOG, PET_BODY_COUNT } PetBody;
typedef enum { PET_FACE_PLAIN, PET_FACE_MONOCLE, PET_FACE_COUNT } PetFace;
typedef enum { PET_FINISH_BRASS, PET_FINISH_COPPER, PET_FINISH_PEWTER, PET_FINISH_VERDIGRIS, PET_FINISH_COUNT } PetFinish;
// "wound up" (break) uses pet.js's love mood
// "sad": the laptop went silent and the session is paused (the dashboard shows it too)
// "hungry": not fed for a day (app/pet_care.hpp). Not in pet.js yet: see docs/PET_PLAY_AND_CARE.md
typedef enum { PET_MOOD_HAPPY, PET_MOOD_LOVE, PET_MOOD_FOCUS, PET_MOOD_ANNOYED, PET_MOOD_SHH, PET_MOOD_SLEEP, PET_MOOD_SAD,
               PET_MOOD_HUNGRY, PET_MOOD_COUNT } PetMood;

#define PET_GRID 16
#define PET_LEVEL_GOGGLES 5
#define PET_LEVEL_TIE     10
#define PET_LEVEL_HAT     15

#define PET_N(a) ((uint8_t)(sizeof(a) / sizeof((a)[0])))
#define PET_PAINT(slot, rects) { (slot), (rects), PET_N(rects) }
#define PET_LAYER(paints) { (paints), PET_N(paints) }

// ---- Base: body + line ----
static const PetRect pet_body_r[] = { {5, 4, 6, 1}, {4, 5, 8, 1}, {3, 6, 10, 6}, {4, 12, 8, 1} };
static const PetRect pet_line_r[] = {
    {5, 3, 6, 1}, {4, 4, 1, 1}, {11, 4, 1, 1}, {3, 5, 1, 1}, {12, 5, 1, 1}, {2, 6, 1, 6},
    {13, 6, 1, 6}, {3, 12, 1, 1}, {12, 12, 1, 1}, {4, 13, 8, 1}, {5, 14, 2, 1}, {9, 14, 2, 1} };
static const PetPaint pet_base_p[] = { PET_PAINT(PET_SLOT_BODY, pet_body_r), PET_PAINT(PET_SLOT_LINE, pet_line_r) };

// ---- Metal ----
static const PetRect pet_metal_shade_r[] = { {4, 12, 8, 1}, {3, 11, 1, 1}, {12, 11, 1, 1} };
static const PetRect pet_metal_hi_r[] = { {5, 5, 2, 1}, {4, 6, 1, 1} };
static const PetRect pet_metal_rivet_r[] = { {3, 7, 1, 1}, {12, 7, 1, 1} };
static const PetPaint pet_metal_p[] = {
    PET_PAINT(PET_SLOT_SHADE, pet_metal_shade_r), PET_PAINT(PET_SLOT_HI, pet_metal_hi_r),
    PET_PAINT(PET_SLOT_RIVET, pet_metal_rivet_r) };

// ---- Bot: wind-up key ----
static const PetRect pet_key_shade_r[] = { {5, 0, 2, 2}, {9, 0, 2, 2} };
static const PetRect pet_key_line_r[] = { {7, 0, 2, 3} };
static const PetPaint pet_key_p[] = { PET_PAINT(PET_SLOT_SHADE, pet_key_shade_r), PET_PAINT(PET_SLOT_LINE, pet_key_line_r) };

// The key turning, seen from the front: full bow (the frame above), three-quarter, edge-on.
// Not in pet.js: the Playground winds it, on every body.
static const PetRect pet_key34_shade_r[] = { {6, 0, 1, 2}, {9, 0, 1, 2} };
static const PetPaint pet_key34_p[] = { PET_PAINT(PET_SLOT_SHADE, pet_key34_shade_r), PET_PAINT(PET_SLOT_LINE, pet_key_line_r) };
static const PetPaint pet_key_edge_p[] = { PET_PAINT(PET_SLOT_LINE, pet_key_line_r) };
#define PET_KEY_FRAMES 4    // full, 3/4, edge, 3/4

// ---- Cat ----
static const PetRect pet_cat_line_r[] = {
    {3, 1, 1, 4}, {4, 2, 1, 1}, {12, 1, 1, 4}, {11, 2, 1, 1}, {0, 9, 2, 1}, {0, 11, 2, 1},
    {14, 9, 2, 1}, {14, 11, 2, 1} };
static const PetRect pet_cat_cheek_r[] = { {4, 3, 1, 1}, {11, 3, 1, 1} };
static const PetRect pet_cat_body_r[] = { {4, 4, 1, 1}, {11, 4, 1, 1} };
static const PetPaint pet_cat_p[] = {
    PET_PAINT(PET_SLOT_LINE, pet_cat_line_r), PET_PAINT(PET_SLOT_CHEEK, pet_cat_cheek_r),
    PET_PAINT(PET_SLOT_BODY, pet_cat_body_r) };

// ---- Dog ----
static const PetRect pet_dog_line_r[] = {
    {3, 2, 2, 1}, {2, 3, 1, 1}, {1, 4, 1, 1}, {0, 5, 1, 4}, {1, 9, 2, 1}, {11, 2, 2, 1},
    {13, 3, 1, 1}, {14, 4, 1, 1}, {15, 5, 1, 4}, {13, 9, 2, 1}, {7, 9, 2, 1} };
static const PetRect pet_dog_shade_r[] = {
    {3, 3, 2, 1}, {2, 4, 2, 1}, {1, 5, 2, 4}, {11, 3, 2, 1}, {12, 4, 2, 1}, {13, 5, 2, 4} };
static const PetPaint pet_dog_p[] = { PET_PAINT(PET_SLOT_LINE, pet_dog_line_r), PET_PAINT(PET_SLOT_SHADE, pet_dog_shade_r) };

// ---- Moods ----
static const PetRect pet_happy_line_r[] = {
    {4, 8, 1, 1}, {5, 7, 1, 1}, {6, 8, 1, 1}, {9, 8, 1, 1}, {10, 7, 1, 1}, {11, 8, 1, 1},
    {6, 10, 1, 1}, {7, 11, 2, 1}, {9, 10, 1, 1} };
static const PetRect pet_happy_cheek_r[] = { {4, 10, 1, 1}, {11, 10, 1, 1} };
static const PetPaint pet_mood_happy_p[] = { PET_PAINT(PET_SLOT_LINE, pet_happy_line_r), PET_PAINT(PET_SLOT_CHEEK, pet_happy_cheek_r) };

static const PetRect pet_love_cheek_r[] = { {3, 10, 2, 1}, {11, 10, 2, 1}, {4, 9, 1, 1}, {11, 9, 1, 1} };
static const PetPaint pet_mood_love_p[] = { PET_PAINT(PET_SLOT_LINE, pet_happy_line_r), PET_PAINT(PET_SLOT_CHEEK, pet_love_cheek_r) };

static const PetRect pet_focus_line_r[] = { {5, 7, 2, 2}, {9, 7, 2, 2}, {7, 10, 2, 1} };
static const PetRect pet_focus_body_r[] = { {5, 7, 1, 1}, {9, 7, 1, 1} };
static const PetPaint pet_mood_focus_p[] = { PET_PAINT(PET_SLOT_LINE, pet_focus_line_r), PET_PAINT(PET_SLOT_BODY, pet_focus_body_r) };

static const PetRect pet_annoyed_line_r[] = {
    {4, 6, 2, 1}, {6, 7, 1, 1}, {10, 6, 2, 1}, {9, 7, 1, 1}, {5, 8, 2, 1}, {9, 8, 2, 1},
    {7, 10, 2, 1}, {6, 11, 1, 1}, {9, 11, 1, 1} };
static const PetPaint pet_mood_annoyed_p[] = { PET_PAINT(PET_SLOT_LINE, pet_annoyed_line_r) };

static const PetRect pet_shh_line_r[] = { {4, 8, 3, 1}, {9, 8, 3, 1}, {7, 10, 2, 2} };
static const PetPaint pet_mood_shh_p[] = { PET_PAINT(PET_SLOT_LINE, pet_shh_line_r), PET_PAINT(PET_SLOT_CHEEK, pet_happy_cheek_r) };

static const PetRect pet_sleep_line_r[] = { {4, 8, 3, 1}, {9, 8, 3, 1}, {7, 10, 2, 1} };
static const PetPaint pet_mood_sleep_p[] = { PET_PAINT(PET_SLOT_LINE, pet_sleep_line_r) };

static const PetRect pet_sad_line_r[] = {
    {6, 6, 1, 1}, {5, 7, 1, 1}, {9, 6, 1, 1}, {10, 7, 1, 1}, {5, 8, 2, 1}, {9, 8, 2, 1},
    {7, 10, 2, 1}, {6, 11, 1, 1}, {9, 11, 1, 1} };
static const PetRect pet_sad_hi_r[] = { {4, 9, 1, 1} };   // a tear
static const PetPaint pet_mood_sad_p[] = { PET_PAINT(PET_SLOT_LINE, pet_sad_line_r), PET_PAINT(PET_SLOT_HI, pet_sad_hi_r) };

// Worried brows, tired eyes, a small open mouth
static const PetRect pet_hungry_line_r[] = {
    {5, 6, 1, 1}, {10, 6, 1, 1}, {5, 7, 2, 1}, {9, 7, 2, 1},
    {7, 10, 2, 1}, {6, 11, 1, 1}, {9, 11, 1, 1}, {7, 12, 2, 1} };
static const PetPaint pet_mood_hungry_p[] = { PET_PAINT(PET_SLOT_LINE, pet_hungry_line_r) };

// ---- Gear (a treat, not part of the pet): 7x7, two frames so it seems to tumble ----
#define PET_GEAR_GRID 7
static const PetRect pet_gear_a_r[] = {   // teeth square
    {3, 0, 1, 1}, {1, 1, 5, 1}, {1, 2, 2, 1}, {4, 2, 2, 1}, {0, 3, 2, 1}, {5, 3, 2, 1},
    {1, 4, 2, 1}, {4, 4, 2, 1}, {1, 5, 5, 1}, {3, 6, 1, 1} };
static const PetRect pet_gear_b_r[] = {   // teeth diagonal
    {1, 0, 1, 1}, {5, 0, 1, 1}, {2, 1, 3, 1}, {1, 2, 2, 1}, {4, 2, 2, 1}, {1, 3, 1, 1},
    {5, 3, 1, 1}, {1, 4, 2, 1}, {4, 4, 2, 1}, {2, 5, 3, 1}, {1, 6, 1, 1}, {5, 6, 1, 1} };

// ---- Accessories ----
static const PetRect pet_goggles_shade_r[] = { {3, 5, 2, 1}, {7, 5, 2, 1}, {11, 5, 2, 1} };
static const PetRect pet_goggles_mono_r[] = { {5, 4, 2, 2}, {9, 4, 2, 2} };
static const PetPaint pet_goggles_p[] = { PET_PAINT(PET_SLOT_SHADE, pet_goggles_shade_r), PET_PAINT(PET_SLOT_MONO, pet_goggles_mono_r) };

static const PetRect pet_tie_cheek_r[] = { {5, 12, 2, 2}, {9, 12, 2, 2} };
static const PetRect pet_tie_line_r[] = { {7, 12, 2, 1} };
static const PetPaint pet_tie_p[] = { PET_PAINT(PET_SLOT_CHEEK, pet_tie_cheek_r), PET_PAINT(PET_SLOT_LINE, pet_tie_line_r) };

static const PetRect pet_hat_line_r[] = { {5, 0, 6, 3}, {4, 3, 8, 1} };
static const PetRect pet_hat_cheek_r[] = { {5, 2, 6, 1} };
static const PetPaint pet_hat_p[] = { PET_PAINT(PET_SLOT_LINE, pet_hat_line_r), PET_PAINT(PET_SLOT_CHEEK, pet_hat_cheek_r) };

static const PetRect pet_monocle_mono_r[] = {
    {8, 6, 5, 1}, {8, 10, 5, 1}, {8, 7, 1, 3}, {12, 7, 1, 3}, {13, 11, 1, 1}, {14, 12, 1, 1} };
static const PetPaint pet_monocle_p[] = { PET_PAINT(PET_SLOT_MONO, pet_monocle_mono_r) };

// ---- Layers ----
static const PetLayer pet_layer_base = PET_LAYER(pet_base_p);
static const PetLayer pet_layer_metal = PET_LAYER(pet_metal_p);
static const PetLayer pet_layer_body[PET_BODY_COUNT] = { PET_LAYER(pet_key_p), PET_LAYER(pet_cat_p), PET_LAYER(pet_dog_p) };
static const PetLayer pet_layer_key[PET_KEY_FRAMES] = {
    PET_LAYER(pet_key_p), PET_LAYER(pet_key34_p), PET_LAYER(pet_key_edge_p), PET_LAYER(pet_key34_p) };
static const PetLayer pet_layer_mood[PET_MOOD_COUNT] = {
    PET_LAYER(pet_mood_happy_p), PET_LAYER(pet_mood_love_p), PET_LAYER(pet_mood_focus_p),
    PET_LAYER(pet_mood_annoyed_p), PET_LAYER(pet_mood_shh_p), PET_LAYER(pet_mood_sleep_p),
    PET_LAYER(pet_mood_sad_p), PET_LAYER(pet_mood_hungry_p) };
static const PetLayer pet_layer_goggles = PET_LAYER(pet_goggles_p);
static const PetLayer pet_layer_tie = PET_LAYER(pet_tie_p);
static const PetLayer pet_layer_hat = PET_LAYER(pet_hat_p);
static const PetLayer pet_layer_monocle = PET_LAYER(pet_monocle_p);

// ---- Palettes (0xRRGGBB), indexed by PetSlot ----
static const uint32_t pet_palettes[PET_FINISH_COUNT][PET_SLOT_COUNT] = {
    //  body      line      cheek     shade     hi        rivet     mono
    { 0xE2BC72, 0x2A1A10, 0xB5462A, 0x8A6430, 0xF4DDA0, 0x6B4A22, 0xE6E8E3 }, // brass
    { 0xD9875A, 0x2A1A10, 0x8E2F1E, 0x8E4A2A, 0xF2B992, 0x6A3018, 0xF4DDA0 }, // copper
    { 0xB9BCB6, 0x1E1E1E, 0xB5462A, 0x6E726C, 0xE6E8E3, 0x4E524C, 0xE2BC72 }, // pewter
    { 0x7FBFAE, 0x12231F, 0xC0563A, 0x3F7A6C, 0xC2E6DC, 0x2E5A50, 0xE2BC72 }, // verdigris
};

#ifdef __cplusplus
}
#endif
