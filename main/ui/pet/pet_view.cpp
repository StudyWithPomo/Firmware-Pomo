#include "pet_view.hpp"

#include <ui/app_state.hpp>

struct PetView {
    PetLook look;
    bool follow_state;
};

static void draw_layer(lv_layer_t *layer, const lv_area_t &origin, int32_t scale,
                       const PetLayer &pet_layer, const uint32_t *palette)
{
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_opa = LV_OPA_COVER;
    dsc.radius = 0;

    for (uint8_t p = 0; p < pet_layer.count; p++) {
        const PetPaint &paint = pet_layer.paints[p];
        dsc.bg_color = lv_color_hex(palette[paint.slot]);
        for (uint8_t r = 0; r < paint.count; r++) {
            const PetRect &rect = paint.rects[r];
            lv_area_t area;
            area.x1 = origin.x1 + rect.x * scale;
            area.y1 = origin.y1 + rect.y * scale;
            area.x2 = area.x1 + rect.w * scale - 1;
            area.y2 = area.y1 + rect.h * scale - 1;
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
    const int32_t scale = LV_MAX(1, LV_MIN(lv_area_get_width(&coords), lv_area_get_height(&coords)) / PET_GRID);
    // Centre the grid if the size isn't an exact multiple of 16
    lv_area_t origin = coords;
    origin.x1 += (lv_area_get_width(&coords) - scale * PET_GRID) / 2;
    origin.y1 += (lv_area_get_height(&coords) - scale * PET_GRID) / 2;

    lv_layer_t *layer = lv_event_get_layer(e);
    const uint32_t *palette = pet_palettes[look.finish < PET_FINISH_COUNT ? look.finish : 0];

    draw_layer(layer, origin, scale, pet_layer_base, palette);
    draw_layer(layer, origin, scale, pet_layer_metal, palette);
    draw_layer(layer, origin, scale, pet_layer_body[look.body < PET_BODY_COUNT ? look.body : 0], palette);
    draw_layer(layer, origin, scale, pet_layer_mood[look.mood < PET_MOOD_COUNT ? look.mood : 0], palette);
    if (look.level >= PET_LEVEL_GOGGLES) draw_layer(layer, origin, scale, pet_layer_goggles, palette);
    if (look.level >= PET_LEVEL_TIE) draw_layer(layer, origin, scale, pet_layer_tie, palette);
    if (look.level >= PET_LEVEL_HAT) draw_layer(layer, origin, scale, pet_layer_hat, palette);
    if (look.face == PET_FACE_MONOCLE) draw_layer(layer, origin, scale, pet_layer_monocle, palette);
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

    lv_obj_t *view = lv_obj_create(parent);
    lv_obj_remove_style_all(view);
    lv_obj_remove_flag(view, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(view, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(view, size, size);
    lv_obj_set_user_data(view, pv);
    lv_obj_add_event_cb(view, on_draw, LV_EVENT_DRAW_MAIN, nullptr);
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

const char *pet_mood_name(int mood)
{
    static const char *const NAMES[PET_MOOD_COUNT] = {"Happy", "Delighted", "Focused", "Annoyed", "Hushed", "Sleepy", "Lonely"};
    return mood >= 0 && mood < PET_MOOD_COUNT ? NAMES[mood] : "";
}
