#pragma once
// Progress and settings that survive a restart, in the NVS namespace "pomo".
// Writes only keys whose value changed, so calling save() often doesn't wear the flash.
#include <stdint.h>

#include <app/session_core.hpp>

struct SavedState {
    int xp;
    int pet_body, pet_face, pet_finish;
    char pet_name[11];
    char user_name[17];
    int study_min, break_min;
    bool always_show_timer;
    SessionLog log;
};

// Defaults for anything missing (a fresh device): Lv 1, Mochi the brass bot, 25 / 5, an
// empty log
void storage_defaults(SavedState *out);

// Read everything saved, falling back to defaults per key. NVS must be initialized.
void storage_load(SavedState *out);

// Write whatever differs from the last load/save. Returns false on a flash error.
bool storage_save(const SavedState &state);
