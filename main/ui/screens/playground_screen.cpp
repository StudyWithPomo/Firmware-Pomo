// Playground: play with the pet. Nothing here earns XP or touches the session log.
//   Wind:  each knob detent (or a tap on Wind, held to repeat) turns the key on top of the pet's
//          head one click and fills the gauge. Every body gets a key here. Left alone, it runs
//          down. Fully wound, the pet dances for 3 s (spins, hops, hearts, a tune), then unwinds.
//          A hungry pet dances less: 2 turns, no hearts.
//   Feed:  a knob press (or Feed a gear) spends one of the gears studying earned and tosses it;
//          the pet catches it with a crunch. No gears, or stuffed (3 in 30 min): it says so.
//   Pet:   tap or stroke it, as everywhere else.
// What it says when idle follows how fed it is. Spec: docs/PET_PLAY_AND_CARE.md
// Nobody playing for 20 s and it nods off; any play wakes it.
// The knob only winds and feeds here (it has no stops to move between): hold to go back.

#include <ui/screens.hpp>
#include <ui/navigator.hpp>
#include <ui/app_state.hpp>
#include <ui/theme.hpp>
#include <ui/pet/pet_view.hpp>
#include <hal/buzzer.h>

#include "lvgl.h"

using namespace theme;

static const int WIND_FULL = 12;                // clicks to a full wind
static const uint32_t DANCE_MS = 3000;
static const int DANCE_TURNS = 3;
static const int HUNGRY_DANCE_TURNS = 2;
static const uint32_t RUN_DOWN_AFTER_MS = 3000; // left alone this long, it starts unwinding
static const uint32_t RUN_DOWN_STEP_MS = 600;   // then loses a click this often
static const uint32_t SLEEP_AFTER_MS = 20000;
static const uint32_t DELIGHT_MS = 1500;        // the love face after a catch or a dance
static const uint32_t GEAR_FLIGHT_MS = 650;
static const int GEAR_ARC_PX = 45;

static struct {
    lv_obj_t *port;
    lv_obj_t *pet;
    lv_obj_t *says;     // the speech bubble's label
    lv_obj_t *fill;     // the wind gauge
    lv_obj_t *feed_btn;
    int dance_turns;
    bool feeding_hungry; // the gear in flight is going to a hungry pet
    lv_obj_t *gear;     // in flight, on the top layer
    lv_timer_t *timer;
    int wind;           // 0..WIND_FULL
    int key_turns;      // the key's frame, 0..PET_KEY_FRAMES-1
    bool dancing;
    bool asleep;
    uint32_t last_play_ms;
    uint32_t last_run_down_ms;
    uint32_t delighted_until_ms;
} ui;

// ---- Mood and words ----

static void say(const char *text) { if (ui.says) lv_label_set_text(ui.says, text); }

static Hunger hunger() { return (Hunger)lv_subject_get_int(&AppState::instance().hunger); }

// What it says with nothing going on, by how fed it is
static const char *idle_line()
{
    switch (hunger()) {
        case Hunger::FULL: return "I'm stuffed! Wind me up instead?";
        case Hunger::PECKISH: return "A gear would be lovely...";
        case Hunger::HUNGRY: return "My tummy's ticking... got a gear?";
        default: return "Wind my key, or toss me a gear!";
    }
}

static void refresh_mood()
{
    if (!ui.pet) return;
    // AppState's idle mood: petted or full -> love, hungry -> hungry, otherwise happy
    PetMood mood = (PetMood)lv_subject_get_int(&AppState::instance().pet_mood);
    if (ui.dancing || lv_tick_get() < ui.delighted_until_ms) mood = PET_MOOD_LOVE;
    else if (ui.asleep) mood = PET_MOOD_SLEEP;
    pet_view_set_mood(ui.pet, mood);
}

// Any play: wakes it and holds off the run-down
static void played()
{
    ui.last_play_ms = lv_tick_get();
    if (ui.asleep) {
        ui.asleep = false;
        say("Oh! Hello again.");
    }
    refresh_mood();
}

static void delight()
{
    ui.delighted_until_ms = lv_tick_get() + DELIGHT_MS;
    refresh_mood();
}

static void show_wind()
{
    if (ui.fill) lv_obj_set_width(ui.fill, LV_PCT(ui.wind * 100 / WIND_FULL));
}

// ---- Dance ----

static void dance_exec(void *var, int32_t t)
{
    if (!ui.pet) return;
    // Spinning: squeezed to nothing and mirrored every half turn
    const int angle = (int)(t * 360 * ui.dance_turns / (int32_t)DANCE_MS) % 360;
    const int32_t cos_v = lv_trigo_cos((int16_t)angle);
    pet_view_set_squash(ui.pet, (int)(LV_ABS(cos_v) * 1000 / LV_TRIGO_SIN_MAX), cos_v < 0);
    // A hop on every half turn
    const int32_t sin_2 = lv_trigo_sin((int16_t)((angle * 2) % 360));
    lv_obj_set_style_translate_y(ui.port, -(int32_t)(LV_ABS(sin_2) * 6 / LV_TRIGO_SIN_MAX), 0);
    // The key spins fast and the gauge drains
    pet_view_set_key(ui.pet, (int)(t / 60));
    ui.wind = WIND_FULL - (int)(t * WIND_FULL / (int32_t)DANCE_MS);
    show_wind();
}

static void dance_done(lv_anim_t *a)
{
    ui.dancing = false;
    ui.wind = 0;
    show_wind();
    if (!ui.pet) return;
    pet_view_set_squash(ui.pet, 1000, false);
    pet_view_set_key(ui.pet, 0);
    lv_obj_set_style_translate_y(ui.port, 0, 0);
    if (hunger() != Hunger::HUNGRY) pet_hearts(ui.port);
    say(hunger() == Hunger::HUNGRY ? "Phew... a gear would help." : "Again! Again!");
    played();
    delight();
}

static void dance()
{
    ui.dancing = true;
    const bool hungry = hunger() == Hunger::HUNGRY;
    ui.dance_turns = hungry ? HUNGRY_DANCE_TURNS : DANCE_TURNS;
    say(hungry ? "Wheee... huff..." : "Wheee!");
    if (!hungry) pet_hearts(ui.port); // plays its coo...
    buzzer_play(BUZZ_DANCE);          // ...which the tune then replaces (same priority)
    refresh_mood();

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui.pet);
    lv_anim_set_exec_cb(&a, dance_exec);
    lv_anim_set_values(&a, 0, DANCE_MS);
    lv_anim_set_duration(&a, DANCE_MS);
    lv_anim_set_completed_cb(&a, dance_done);
    lv_anim_start(&a);
}

// ---- Winding ----

static void wind_click()
{
    if (ui.dancing || !ui.pet) return;
    played();
    buzzer_play(BUZZ_WIND);
    ui.key_turns = (ui.key_turns + 1) % PET_KEY_FRAMES;
    pet_view_set_key(ui.pet, ui.key_turns);
    if (ui.wind < WIND_FULL) ui.wind++;
    show_wind();
    if (ui.wind >= WIND_FULL) dance();
    else say(ui.wind < WIND_FULL / 2 ? "Keep winding..." : "Almost there!");
}

static void run_down()
{
    if (ui.wind == 0 || ui.dancing) return;
    ui.wind--;
    ui.key_turns = (ui.key_turns + PET_KEY_FRAMES - 1) % PET_KEY_FRAMES; // the key turns back
    pet_view_set_key(ui.pet, ui.key_turns);
    show_wind();
    if (ui.wind == 0) say(idle_line());
}

// ---- Gears ----

static const int GEAR_GRID = PET_GEAR_GRID, GEAR_SCALE = 3;

static void gear_draw(lv_event_t *e)
{
    lv_obj_t *gear = lv_event_get_current_target_obj(e);
    const bool frame_b = (uintptr_t)lv_obj_get_user_data(gear) & 1;
    const PetRect *rects = frame_b ? pet_gear_b_r : pet_gear_a_r;
    const int count = frame_b ? PET_N(pet_gear_b_r) : PET_N(pet_gear_a_r);

    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = c(BRASS_HI);
    dsc.bg_opa = LV_OPA_COVER;
    lv_area_t coords;
    lv_obj_get_coords(gear, &coords);
    for (int i = 0; i < count; i++) {
        lv_area_t a;
        a.x1 = coords.x1 + rects[i].x * GEAR_SCALE;
        a.y1 = coords.y1 + rects[i].y * GEAR_SCALE;
        a.x2 = a.x1 + rects[i].w * GEAR_SCALE - 1;
        a.y2 = a.y1 + rects[i].h * GEAR_SCALE - 1;
        lv_draw_rect(lv_event_get_layer(e), &dsc, &a);
    }
}

static struct { lv_point_t from, to; } s_flight;

// t runs 0..1000: straight across, plus an arc up and down
static void gear_exec(void *var, int32_t t)
{
    lv_obj_t *gear = (lv_obj_t *)var;
    const int32_t x = s_flight.from.x + (s_flight.to.x - s_flight.from.x) * t / 1000;
    const int32_t arc = GEAR_ARC_PX * lv_trigo_sin((int16_t)(t * 180 / 1000)) / LV_TRIGO_SIN_MAX;
    const int32_t y = s_flight.from.y + (s_flight.to.y - s_flight.from.y) * t / 1000 - arc;
    lv_obj_set_pos(gear, x, y);
    lv_obj_set_user_data(gear, (void *)(uintptr_t)(t / 90)); // tumble
    lv_obj_invalidate(gear);
}

static void gear_caught(lv_anim_t *a)
{
    lv_obj_delete_async((lv_obj_t *)a->var);
    if (ui.gear != a->var) return; // the Playground was left while it flew
    ui.gear = nullptr;
    buzzer_play(BUZZ_CRUNCH);
    pet_squish(ui.pet);
    say(ui.feeding_hungry ? "Oh, that's better!" : "Crunchy! Thank you.");
    played();
    delight();
}

static void feed()
{
    if (ui.gear || ui.dancing || !ui.pet) return; // one gear at a time
    played();
    ui.feeding_hungry = hunger() == Hunger::HUNGRY;
    switch (AppState::instance().feed()) {
        case FeedResult::EMPTY:
            buzzer_play(BUZZ_SHH);
            say("Out of gears! Finish a session to earn some.");
            return;
        case FeedResult::STUFFED:
            buzzer_play(BUZZ_SHH);
            say("I'm stuffed! Save it for later.");
            return;
        default:
            break;
    }
    buzzer_play(BUZZ_TOSS);
    say("Ooh, a gear!");

    // From the Feed button up and over into the pet's mouth
    lv_area_t btn, pet;
    lv_obj_get_coords(ui.feed_btn, &btn);
    lv_obj_get_coords(ui.pet, &pet);
    const int32_t half = GEAR_GRID * GEAR_SCALE / 2;
    s_flight.from = { (btn.x1 + btn.x2) / 2 - half, btn.y1 - half };
    s_flight.to = { (pet.x1 + pet.x2) / 2 - half, pet.y1 + lv_area_get_height(&pet) * 5 / 8 - half };

    ui.gear = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(ui.gear);
    lv_obj_remove_flag(ui.gear, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(ui.gear, GEAR_GRID * GEAR_SCALE, GEAR_GRID * GEAR_SCALE);
    lv_obj_set_pos(ui.gear, s_flight.from.x, s_flight.from.y);
    lv_obj_add_event_cb(ui.gear, gear_draw, LV_EVENT_DRAW_MAIN, nullptr);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui.gear);
    lv_anim_set_exec_cb(&a, gear_exec);
    lv_anim_set_values(&a, 0, 1000);
    lv_anim_set_duration(&a, GEAR_FLIGHT_MS);
    lv_anim_set_completed_cb(&a, gear_caught);
    lv_anim_start(&a);
}

// ---- Input ----

static void on_knob_key(lv_event_t *e)
{
    const uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_RIGHT || key == LV_KEY_LEFT) wind_click(); // either way winds
}

static void on_knob_press(lv_event_t *e) { feed(); }
static void on_wind_btn(lv_event_t *e) { wind_click(); }
static void on_feed_btn(lv_event_t *e) { feed(); }
static bool can_pet() { return !ui.dancing; }

static void update_mood(lv_observer_t *observer, lv_subject_t *subject) { refresh_mood(); }

// Petting counts as play (pettable() does the hearts; this wakes it and holds off the run-down)
static void on_petting(lv_event_t *e)
{
    if (!ui.dancing) played();
}

static void tick(lv_timer_t *t)
{
    const uint32_t idle = lv_tick_elaps(ui.last_play_ms);
    if (idle >= RUN_DOWN_AFTER_MS && lv_tick_elaps(ui.last_run_down_ms) >= RUN_DOWN_STEP_MS) {
        ui.last_run_down_ms = lv_tick_get();
        run_down();
    }
    if (!ui.asleep && !ui.dancing && !ui.gear && ui.wind == 0 && idle >= SLEEP_AFTER_MS) {
        ui.asleep = true;
        say("Zzz...");
    }
    refresh_mood();
}

// No gears: Feed looks spent (dim outline and text; an opacity would need a layer buffer)
static void update_feed_btn(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *btn = lv_observer_get_target_obj(observer);
    const bool any = lv_subject_get_int(subject) > 0;
    lv_obj_set_style_border_color(btn, c(any ? BRASS : TUBE_RIM), 0);
    lv_obj_t *text = lv_obj_get_child(lv_obj_get_child(btn, 0), 1);
    lv_obj_set_style_text_color(text, c(any ? BRASS : MUTED), 0);
}

static void on_delete(lv_event_t *e)
{
    if (ui.timer) lv_timer_delete(ui.timer);
    // A gear in flight lives on the top layer: stop it before it lands on a screen that's gone
    if (ui.gear) {
        lv_anim_delete(ui.gear, gear_exec);
        lv_obj_delete_async(ui.gear);
    }
    ui = {};
}

// A touch-only button: the knob winds and feeds directly
static lv_obj_t *touch_button(lv_obj_t *parent, const char *text, int width)
{
    lv_obj_t *btn = button(parent, nullptr, text, &font_fell_14);
    lv_obj_set_size(btn, width, 30);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_CLICK_FOCUSABLE); // a tap shouldn't leave it looking focused
    lv_obj_set_style_bg_color(btn, c(BRASS), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_40, LV_STATE_PRESSED);
    return btn;
}

void screen_playground(lv_obj_t *parent, lv_group_t *group)
{
    ui = {};
    ui.last_play_ms = lv_tick_get();
    lv_obj_t *content = frame(parent);
    lv_obj_add_event_cb(content, on_delete, LV_EVENT_DELETE, nullptr);

    Header top = header(content, "Playground");
    gears_bar(top.row);

    // Middle: the pet with its key, and what it says over the wind gauge
    lv_obj_t *middle = row(content, LV_FLEX_ALIGN_START, 12);
    lv_obj_set_width(middle, LV_PCT(100));
    ui.port = porthole(middle, 112);
    ui.pet = pet_view_create(ui.port, 96, PET_MOOD_HAPPY);
    pet_view_set_key(ui.pet, 0);
    pettable(ui.pet, ui.port, can_pet);
    lv_obj_add_event_cb(ui.pet, on_petting, LV_EVENT_PRESSED, nullptr);
    lv_subject_add_observer_obj(&AppState::instance().pet_mood, update_mood, ui.pet, nullptr);

    lv_obj_t *side = column(middle, 8);
    lv_obj_set_flex_grow(side, 1);
    lv_obj_t *bubble_box = bubble(side, idle_line(), 150);
    ui.says = lv_obj_get_child(bubble_box, 0);
    label(side, "Wound up", &font_fell_12, MUTED);
    lv_obj_t *rail = box(side);
    lv_obj_set_size(rail, 150, 9);
    lv_obj_set_style_border_width(rail, 1, 0);
    lv_obj_set_style_border_color(rail, c(BRASS), 0);
    lv_obj_set_style_pad_all(rail, 1, 0);
    ui.fill = box(rail);
    lv_obj_set_height(ui.fill, LV_PCT(100));
    lv_obj_set_style_bg_opa(ui.fill, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(ui.fill, c(NIXIE), 0);
    show_wind();

    // Bottom: touch buttons
    lv_obj_t *buttons = row(content, LV_FLEX_ALIGN_CENTER, 12);
    lv_obj_set_width(buttons, LV_PCT(100));
    lv_obj_t *wind_btn = touch_button(buttons, "Wind", 110);
    lv_obj_add_event_cb(wind_btn, on_wind_btn, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(wind_btn, on_wind_btn, LV_EVENT_LONG_PRESSED_REPEAT, nullptr);
    ui.feed_btn = touch_button(buttons, "Feed a gear", 130);
    lv_obj_add_event_cb(ui.feed_btn, on_feed_btn, LV_EVENT_CLICKED, nullptr);
    lv_subject_add_observer_obj(&AppState::instance().gears, update_feed_btn, ui.feed_btn, nullptr);

    // The knob's one stop, always turning: detents wind, a press feeds, a hold goes back
    lv_group_add_obj(group, content);
    lv_obj_add_event_cb(content, on_knob_key, LV_EVENT_KEY, nullptr);
    lv_obj_add_event_cb(content, on_knob_press, LV_EVENT_CLICKED, nullptr);
    always_editing(group);

    ui.timer = lv_timer_create(tick, 200, nullptr);
}
