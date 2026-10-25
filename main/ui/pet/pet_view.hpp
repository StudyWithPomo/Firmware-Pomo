#pragma once
#include <stdint.h>

#include "lvgl.h"
#include "pet_data.h"

struct PetLook {
    PetBody body;
    PetFace face;
    PetFinish finish;
    PetMood mood;
    int level;
};

// The pet, drawn straight from the 16x16 rectangle data as scaled rectangles in a draw
// callback: no canvas, no image transform, no buffer to allocate, and crisp pixels at any
// scale. `size` should be a multiple of 16; empty grid cells stay transparent, so whatever
// is behind (normally the porthole glass) shows through.
//
// By default the view follows AppState (body, face, finish, level) and redraws when they
// change; the mood is per screen.
lv_obj_t *pet_view_create(lv_obj_t *parent, int size, PetMood mood);

void pet_view_set_mood(lv_obj_t *view, PetMood mood);

// "Happy", "Wound up", ... for showing the mood in words
const char *pet_mood_name(int mood);

// Show a fixed look instead of following AppState (previews, pet sheets)
void pet_view_set_look(lv_obj_t *view, const PetLook &look);

// Squeeze or stretch the pet sideways about its centre (1000 = as drawn, 0 = edge-on, at
// most 1200), and
// optionally mirror it. Animating this through 0 with a mirror flip looks like a turn, with
// no image buffer: the rectangles are just drawn narrower.
void pet_view_set_squash(lv_obj_t *view, int x_permille, bool mirror);

// A wind-up key on top of the head, on any body, turned to `frame` (0..PET_KEY_FRAMES-1);
// -1 shows the body as pet.js draws it (only the bot has a key)
void pet_view_set_key(lv_obj_t *view, int frame);
