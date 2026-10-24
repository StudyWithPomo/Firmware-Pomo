#pragma once
#include <stdint.h>

#include "lvgl.h"
#include "pet_data.h"
#include <ui/theme.hpp>

struct PetLook {
    PetBody body;
    PetFace face;
    PetFinish finish;
    PetMood mood;
    int level;
};

// Draws the pet into a 16x16 RGB565 canvas (512 B) and upscales it with nearest-neighbour,
// so the pixels stay crisp. `size` should be a multiple of 16. `bg` fills the empty pixels
// and should match what's behind the pet (normally the porthole glass).
//
// By default the view follows AppState (body, face, finish, level) and redraws when they
// change; the mood is per screen.
lv_obj_t *pet_view_create(lv_obj_t *parent, int size, PetMood mood, uint32_t bg = theme::PORTHOLE);

void pet_view_set_mood(lv_obj_t *view, PetMood mood);

// Show a fixed look instead of following AppState (the Workshop's live preview)
void pet_view_set_look(lv_obj_t *view, const PetLook &look);

// Paint a look into any 16x16 RGB565 canvas
void pet_paint(lv_obj_t *canvas, const PetLook &look, uint32_t bg);
