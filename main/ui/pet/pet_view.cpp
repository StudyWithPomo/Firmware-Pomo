#include "pet_view.hpp"

#include <ui/app_state.hpp>

struct PetView {
    lv_draw_buf_t *buf;
    PetLook look;
    uint32_t bg;
    bool follow_state;
};

static void paint_layer(lv_obj_t *canvas, const PetLayer &layer, const uint32_t *palette)
{
    for (uint8_t p = 0; p < layer.count; p++) {
        const PetPaint &paint = layer.paints[p];
        const lv_color_t color = lv_color_hex(palette[paint.slot]);
        for (uint8_t r = 0; r < paint.count; r++) {
            const PetRect &rect = paint.rects[r];
            for (uint8_t y = rect.y; y < rect.y + rect.h; y++)
                for (uint8_t x = rect.x; x < rect.x + rect.w; x++)
                    lv_canvas_set_px(canvas, x, y, color, LV_OPA_COVER);
        }
    }
}

// Same order as drawPet() in pet.js
void pet_paint(lv_obj_t *canvas, const PetLook &look, uint32_t bg)
{
    const uint32_t *palette = pet_palettes[look.finish < PET_FINISH_COUNT ? look.finish : 0];

    lv_canvas_fill_bg(canvas, lv_color_hex(bg), LV_OPA_COVER);
    paint_layer(canvas, pet_layer_base, palette);
    paint_layer(canvas, pet_layer_metal, palette);
    paint_layer(canvas, pet_layer_body[look.body < PET_BODY_COUNT ? look.body : 0], palette);
    paint_layer(canvas, pet_layer_mood[look.mood < PET_MOOD_COUNT ? look.mood : 0], palette);
    if (look.level >= PET_LEVEL_GOGGLES) paint_layer(canvas, pet_layer_goggles, palette);
    if (look.level >= PET_LEVEL_TIE) paint_layer(canvas, pet_layer_tie, palette);
    if (look.level >= PET_LEVEL_HAT) paint_layer(canvas, pet_layer_hat, palette);
    if (look.face == PET_FACE_MONOCLE) paint_layer(canvas, pet_layer_monocle, palette);
}

static void redraw(lv_obj_t *view)
{
    PetView *pv = (PetView *)lv_obj_get_user_data(view);
    if (pv) pet_paint(view, pv->look, pv->bg);
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
    redraw(view);
}

static void on_delete(lv_event_t *e)
{
    lv_obj_t *view = lv_event_get_current_target_obj(e);
    PetView *pv = (PetView *)lv_obj_get_user_data(view);
    if (!pv) return;
    lv_obj_set_user_data(view, nullptr);
    lv_draw_buf_destroy(pv->buf);
    lv_free(pv);
}

lv_obj_t *pet_view_create(lv_obj_t *parent, int size, PetMood mood, uint32_t bg)
{
    PetView *pv = (PetView *)lv_malloc(sizeof(PetView));
    LV_ASSERT_MALLOC(pv);
    pv->buf = lv_draw_buf_create(PET_GRID, PET_GRID, LV_COLOR_FORMAT_RGB565, LV_STRIDE_AUTO);
    LV_ASSERT_MALLOC(pv->buf);
    pv->look = { PET_BODY_BOT, PET_FACE_PLAIN, PET_FINISH_BRASS, mood, 1 };
    pv->bg = bg;
    pv->follow_state = true;

    lv_obj_t *view = lv_canvas_create(parent);
    lv_canvas_set_draw_buf(view, pv->buf);
    lv_obj_set_user_data(view, pv);
    lv_obj_add_event_cb(view, on_delete, LV_EVENT_DELETE, nullptr);

    // Upscale 16x16 to the requested size without smoothing
    lv_image_set_inner_align(view, LV_IMAGE_ALIGN_STRETCH);
    lv_image_set_antialias(view, false);
    lv_obj_set_size(view, size, size);

    // Each observer fires once on creation, which paints the first frame
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
    redraw(view);
}

void pet_view_set_look(lv_obj_t *view, const PetLook &look)
{
    PetView *pv = (PetView *)lv_obj_get_user_data(view);
    if (!pv) return;
    pv->follow_state = false;
    pv->look = look;
    redraw(view);
}
