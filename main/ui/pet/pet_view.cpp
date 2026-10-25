#include "pet_view.hpp"

#include <ui/app_state.hpp>

struct PetView {
    PetLook look;
    bool follow_state;
    int16_t squash;     // horizontal scale, permille
    bool mirror;
    int8_t key_frame;   // -1: no extra key
};

struct Placement {
    lv_area_t origin;   // top-left of the 16x16 grid
    int32_t scale;      // px per grid cell
    int16_t squash;
    bool mirror;
};

static void draw_layer(lv_layer_t *layer, const Placement &at, const PetLayer &pet_layer, const uint32_t *palette)
{
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_opa = LV_OPA_COVER;
    dsc.radius = 0;
    const int32_t centre = at.origin.x1 + at.scale * PET_GRID / 2;

    for (uint8_t p = 0; p < pet_layer.count; p++) {
        const PetPaint &paint = pet_layer.paints[p];
        dsc.bg_color = lv_color_hex(palette[paint.slot]);
        for (uint8_t r = 0; r < paint.count; r++) {
            const PetRect &rect = paint.rects[r];
            const int x = at.mirror ? PET_GRID - rect.x - rect.w : rect.x;
            int32_t left = at.origin.x1 + x * at.scale;
            int32_t right = left + rect.w * at.scale;
            if (at.squash != 1000) {
                left = centre + (left - centre) * at.squash / 1000;
                right = centre + (right - centre) * at.squash / 1000;
                if (right <= left) continue; // squeezed to nothing
            }
            lv_area_t area;
            area.x1 = left;
            area.x2 = right - 1;
            area.y1 = at.origin.y1 + rect.y * at.scale;
            area.y2 = area.y1 + rect.h * at.scale - 1;
            lv_draw_rect(layer, &dsc, &area);
        }
    }
}

// Same layer order as drawPet() in pet.js
static void on_draw(lv_event_t *e)
{
    lv_obj_t *view = lv_event_get_current_target_obj(e);
    PetView *pv = (PetView *)lv_obj_get_user_data(view);
    if (!pv) return;
    const PetLook &look = pv->look;

    lv_area_t coords;
    lv_obj_get_content_coords(view, &coords);
    Placement at;
    at.scale = LV_MAX(1, LV_MIN(lv_area_get_width(&coords), lv_area_get_height(&coords)) / PET_GRID);
    // Centre the grid if the size isn't an exact multiple of 16
    at.origin = coords;
    at.origin.x1 += (lv_area_get_width(&coords) - at.scale * PET_GRID) / 2;
    at.origin.y1 += (lv_area_get_height(&coords) - at.scale * PET_GRID) / 2;
    at.squash = pv->squash;
    at.mirror = pv->mirror;

    lv_layer_t *layer = lv_event_get_layer(e);
    const uint32_t *palette = pet_palettes[look.finish < PET_FINISH_COUNT ? look.finish : 0];
    const PetBody body = look.body < PET_BODY_COUNT ? look.body : PET_BODY_BOT;

    draw_layer(layer, at, pet_layer_base, palette);
    draw_layer(layer, at, pet_layer_metal, palette);
    // The bot's body layer is its key: with a turning key it's replaced by that frame
    if (pv->key_frame >= 0) {
        if (body != PET_BODY_BOT) draw_layer(layer, at, pet_layer_body[body], palette);
        draw_layer(layer, at, pet_layer_key[pv->key_frame % PET_KEY_FRAMES], palette);
    } else {
        draw_layer(layer, at, pet_layer_body[body], palette);
    }
    draw_layer(layer, at, pet_layer_mood[look.mood < PET_MOOD_COUNT ? look.mood : 0], palette);
    if (look.level >= PET_LEVEL_GOGGLES) draw_layer(layer, at, pet_layer_goggles, palette);
    if (look.level >= PET_LEVEL_TIE) draw_layer(layer, at, pet_layer_tie, palette);
    if (look.level >= PET_LEVEL_HAT) draw_layer(layer, at, pet_layer_hat, palette);
    if (look.face == PET_FACE_MONOCLE) draw_layer(layer, at, pet_layer_monocle, palette);
}

static void on_state_change(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *view = lv_observer_get_target_obj(observer);
    PetView *pv = (PetView *)lv_obj_get_user_data(view);
    if (!pv || !pv->follow_state) return;

    AppState &state = AppState::instance();
    pv->look.body = (PetBody)lv_subject_get_int(&state.pet_body);
    pv->look.face = (PetFace)lv_subject_get_int(&state.pet_face);
    pv->look.finish = (PetFinish)lv_subject_get_int(&state.pet_finish);
    pv->look.level = lv_subject_get_int(&state.level);
    lv_obj_invalidate(view);
}

// Room to draw a pet stretched up to MAX_SQUASH outside its box (the squish when petted)
static const int MAX_SQUASH = 1200;

static void on_ext_draw_size(lv_event_t *e)
{
    lv_obj_t *view = lv_event_get_current_target_obj(e);
    lv_event_set_ext_draw_size(e, lv_obj_get_width(view) * (MAX_SQUASH - 1000) / 2000 + 1);
}

static void on_delete(lv_event_t *e)
{
    lv_obj_t *view = lv_event_get_current_target_obj(e);
    PetView *pv = (PetView *)lv_obj_get_user_data(view);
    lv_obj_set_user_data(view, nullptr);
    lv_free(pv);
}

lv_obj_t *pet_view_create(lv_obj_t *parent, int size, PetMood mood)
{
    PetView *pv = (PetView *)lv_malloc(sizeof(PetView));
    LV_ASSERT_MALLOC(pv);
    pv->look = { PET_BODY_BOT, PET_FACE_PLAIN, PET_FINISH_BRASS, mood, 1 };
    pv->follow_state = true;
    pv->squash = 1000;
    pv->mirror = false;
    pv->key_frame = -1;

    lv_obj_t *view = lv_obj_create(parent);
    lv_obj_remove_style_all(view);
    lv_obj_remove_flag(view, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(view, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(view, size, size);
    lv_obj_set_user_data(view, pv);
    lv_obj_add_event_cb(view, on_draw, LV_EVENT_DRAW_MAIN, nullptr);
    lv_obj_add_event_cb(view, on_ext_draw_size, LV_EVENT_REFR_EXT_DRAW_SIZE, nullptr);
    lv_obj_add_event_cb(view, on_delete, LV_EVENT_DELETE, nullptr);

    // Each observer fires once on creation, which loads the look from AppState
    AppState &state = AppState::instance();
    lv_subject_add_observer_obj(&state.pet_body, on_state_change, view, nullptr);
    lv_subject_add_observer_obj(&state.pet_face, on_state_change, view, nullptr);
    lv_subject_add_observer_obj(&state.pet_finish, on_state_change, view, nullptr);
    lv_subject_add_observer_obj(&state.level, on_state_change, view, nullptr);
    return view;
}

void pet_view_set_mood(lv_obj_t *view, PetMood mood)
{
    PetView *pv = (PetView *)lv_obj_get_user_data(view);
    if (!pv || pv->look.mood == mood) return;
    pv->look.mood = mood;
    lv_obj_invalidate(view);
}

void pet_view_set_look(lv_obj_t *view, const PetLook &look)
{
    PetView *pv = (PetView *)lv_obj_get_user_data(view);
    if (!pv) return;
    pv->follow_state = false;
    pv->look = look;
    lv_obj_invalidate(view);
}

void pet_view_set_squash(lv_obj_t *view, int x_permille, bool mirror)
{
    PetView *pv = (PetView *)lv_obj_get_user_data(view);
    if (!pv) return;
    x_permille = LV_CLAMP(0, x_permille, MAX_SQUASH);
    if (pv->squash == x_permille && pv->mirror == mirror) return;
    pv->squash = (int16_t)x_permille;
    pv->mirror = mirror;
    lv_obj_invalidate(view);
}

void pet_view_set_key(lv_obj_t *view, int frame)
{
    PetView *pv = (PetView *)lv_obj_get_user_data(view);
    if (!pv || pv->key_frame == frame) return;
    pv->key_frame = (int8_t)(frame < 0 ? -1 : frame % PET_KEY_FRAMES);
    lv_obj_invalidate(view);
}

const char *pet_mood_name(int mood)
{
    static const char *const NAMES[PET_MOOD_COUNT] = {"Happy", "Delighted", "Focused", "Annoyed", "Hushed", "Sleepy", "Lonely"};
    return mood >= 0 && mood < PET_MOOD_COUNT ? NAMES[mood] : "";
}
