#include "app_state.hpp"
#include <system/system.hpp>
#include <ui/pet/pet_data.h>

AppState& AppState::instance() {
    static AppState inst;
    return inst;
}

static long current_time_cache = 0;

static char pet_name_buf[11];
static char pet_name_prev[11];
static char circle_names_buf[64];
static char circle_names_prev[64];

AppState::AppState() {
    current_time_cache = SystemAPI::instance().getCurrentTime();
    lv_subject_init_pointer(&time_subject, &current_time_cache);

    lv_subject_init_string(&pet_name, pet_name_buf, pet_name_prev, sizeof(pet_name_buf), "Mochi");
    lv_subject_init_int(&pet_body, PET_BODY_BOT);
    lv_subject_init_int(&pet_face, PET_FACE_PLAIN);
    lv_subject_init_int(&pet_finish, PET_FINISH_BRASS);

    lv_subject_init_int(&xp, 0);
    lv_subject_init_int(&level, 1);

    lv_subject_init_int(&multiplier, 1);
    lv_subject_init_string(&circle_names, circle_names_buf, circle_names_prev, sizeof(circle_names_buf), "");

    lv_subject_init_int(&study_min, 25);
    lv_subject_init_int(&break_min, 5);
}

void AppState::update() {
    const long current_time = SystemAPI::instance().getCurrentTime();
    if (current_time != current_time_cache) {
        current_time_cache = current_time;
        lv_subject_set_pointer(&time_subject, &current_time_cache);
    }
}
