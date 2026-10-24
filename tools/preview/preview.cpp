// Screenshot preview: runs the real Pomo UI (screens, theme, fonts, pet, AppState, Navigator,
// session engine) on a PC against a 320x240 LVGL display and dumps each screen to
// out/<name>.rgb565 for to_png.py. No hardware, no ESP-IDF.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "lvgl.h"
#include "esp_timer.h"

#include "ui/ui_root.hpp"
#include "ui/theme.hpp"
#include "ui/navigator.hpp"
#include "ui/app_state.hpp"
#include "ui/pet/pet_view.hpp"
#include "app/session.hpp"

void sim_encoder_hold(void);
extern long g_sim_epoch;

static const int W = 320, H = 240;
static uint16_t s_fb[W * H];
static uint32_t s_tick_ms = 0;
static std::string s_out_dir = "out";
static FILE *s_index;

static uint32_t tick_cb(void) { return s_tick_ms; }

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px)
{
    const int32_t w = lv_area_get_width(area);
    for (int32_t y = area->y1; y <= area->y2; y++)
        memcpy(&s_fb[y * W + area->x1], px + (size_t)(y - area->y1) * w * 2, (size_t)w * 2);
    lv_display_flush_ready(disp);
}

// Let LVGL run for `ms` of simulated time (async calls, animations, the 50 ms queue drain)
static void pump(uint32_t ms)
{
    for (uint32_t t = 0; t < ms; t += 5) {
        s_tick_ms += 5;
        lv_timer_handler();
    }
}

// Advance the session clock by whole seconds, letting the UI drain in between
static void advance_seconds(int seconds)
{
    for (int i = 0; i < seconds; i++) {
        sim_timers_advance(1000 * 1000);
        g_sim_epoch++;
        if (i % 30 == 29 || i == seconds - 1) pump(60);
    }
}

static void shot(const char *name, const char *caption)
{
    pump(300);
    lv_refr_now(nullptr);

    std::string path = s_out_dir + "/" + name + ".rgb565";
    FILE *f = fopen(path.c_str(), "wb");
    if (!f) { perror(path.c_str()); exit(1); }
    fwrite(s_fb, sizeof(s_fb), 1, f);
    fclose(f);

    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    fprintf(s_index, "%s\t%s\t%u\t%u\n", name, caption, (unsigned)(mon.total_size - mon.free_size),
            (unsigned)mon.total_size);
    printf("shot %-18s LVGL heap used %6u / %u (%u%% frag)\n", name,
           (unsigned)(mon.total_size - mon.free_size), (unsigned)mon.total_size, (unsigned)mon.frag_pct);
}

static void nav_reset(ScreenID id) { Navigator::instance().reset(id); pump(50); }
static void nav_go(ScreenID id) { Navigator::instance().navigate(id); pump(50); }

static void send_key(uint32_t key)
{
    lv_group_t *g = Navigator::instance().currentGroup();
    lv_group_send_data(g, key);
    pump(50);
}

static void click_focused()
{
    lv_group_t *g = Navigator::instance().currentGroup();
    lv_obj_t *obj = lv_group_get_focused(g);
    if (obj) lv_obj_send_event(obj, LV_EVENT_CLICKED, nullptr);
    pump(50);
}

static void focus_next() { lv_group_focus_next(Navigator::instance().currentGroup()); pump(50); }

// A grid of every body x finish, and one of every mood, drawn with the real pet renderer
static void pet_sheet(bool moods)
{
    lv_obj_t *content_area = ui_get_content_area();
    lv_obj_clean(content_area);
    lv_obj_t *content = theme::frame(content_area);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *grid = lv_obj_create(content);
    lv_obj_remove_style_all(grid);
    lv_obj_set_size(grid, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(grid, 6, 0);

    static const char *const finishes[] = {"brass", "copper", "pewter", "verdigris"};
    static const char *const bodies[] = {"bot", "cat", "dog"};
    static const char *const mood_names[] = {"happy", "love", "focus", "annoyed", "shh", "sleep"};

    const int n = moods ? PET_MOOD_COUNT : PET_BODY_COUNT * PET_FINISH_COUNT;
    for (int i = 0; i < n; i++) {
        lv_obj_t *cell = theme::column(grid, 2);
        lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_t *port = theme::porthole(cell, moods ? 58 : 44, 2);
        lv_obj_t *pet = pet_view_create(port, moods ? 48 : 32, PET_MOOD_HAPPY);
        PetLook look = { PET_BODY_BOT, PET_FACE_PLAIN, PET_FINISH_BRASS, PET_MOOD_HAPPY, 1 };
        char name[24];
        if (moods) {
            look.mood = (PetMood)i;
            snprintf(name, sizeof(name), "%s", mood_names[i]);
        } else {
            look.body = (PetBody)(i / PET_FINISH_COUNT);
            look.finish = (PetFinish)(i % PET_FINISH_COUNT);
            snprintf(name, sizeof(name), "%s %s", finishes[look.finish], bodies[look.body]);
        }
        pet_view_set_look(pet, look);
        theme::label(cell, name, &font_caslon_12, theme::MUTED);
    }
}

int main(int argc, char **argv)
{
    if (argc > 1) s_out_dir = argv[1];
    s_index = fopen((s_out_dir + "/index.tsv").c_str(), "w");
    if (!s_index) { perror("index.tsv"); return 1; }

    g_sim_epoch = 1792790040; // Fri 23 Oct 2026, 9:14 PM

    lv_init();
    lv_tick_set_cb(tick_cb);
    lv_display_t *disp = lv_display_create(W, H);
    // Like the device: one 40-row buffer, partial rendering
    static uint8_t buf[W * 40 * 2];
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, buf, nullptr, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, flush_cb);

    // Same start-up order as app_main
    Session::instance().init();
    theme::init();
    ui_root_init();
    Navigator::instance().init(nullptr);
    AppState &state = AppState::instance();
    state.attach();
    lv_subject_copy_string(&state.user_name, "Sidak");
    pump(100);

    nav_reset(ScreenID::BOOT);
    shot("00_boot", "00 Boot");

    nav_reset(ScreenID::HOME);
    shot("01_home_solo", "01 Home, studying solo");

    AppState::Friend friends[] = { {"Ava", true, 0}, {"Dev", true, 0} };
    state.setFriends(friends, 2);
    Session::instance().setCircle(2, false);
    pump(100);
    shot("01_home_circle", "01 Home, with Ava and Dev nearby");
    focus_next();
    shot("01_home_circle_focus", "01 Home, knob on the circle card");

    nav_reset(ScreenID::BOOT);
    click_focused(); // Yes, wind it
    shot("02_wind", "02 Wind the timer (25 / 5)");
    click_focused(); // grab Study
    send_key(LV_KEY_RIGHT);
    shot("02_wind_45", "02 Wind, Study grabbed and turned to 45 min (break range follows)");
    send_key(LV_KEY_LEFT);
    click_focused(); // let go
    focus_next();
    click_focused(); // grab Break
    send_key(LV_KEY_RIGHT);
    shot("02_wind_break", "02 Wind, Break grabbed and turned to 6");
    send_key(LV_KEY_LEFT);
    click_focused();
    focus_next();
    shot("02_wind_start", "02 Wind, knob on Start");

    nav_reset(ScreenID::HOME);
    nav_go(ScreenID::MENU);
    shot("06_menu", "06 Menu");

    nav_go(ScreenID::WORKSHOP);
    state.setPet(PET_BODY_CAT, PET_FACE_MONOCLE, PET_FINISH_COPPER);
    pump(50);
    shot("07_workshop", "07 Workshop (copper cat with monocle)");
    click_focused(); // enter the Body row
    shot("07_workshop_edit", "07 Workshop, editing the Body row");
    click_focused();
    state.setPet(PET_BODY_BOT, PET_FACE_PLAIN, PET_FINISH_BRASS);
    focus_next();
    focus_next();
    focus_next();
    shot("07_workshop_back", "07 Workshop, knob on Back");

    nav_reset(ScreenID::HOME);
    nav_go(ScreenID::CIRCLE);
    shot("08_circle", "Study circle");

    nav_reset(ScreenID::HOME);
    nav_go(ScreenID::TIMER_SETTINGS);
    shot("09_timer_settings", "Timer settings");
    click_focused();
    shot("09_timer_settings_edit", "Timer settings, Study grabbed");
    click_focused();

    // A real session: Begin on Home
    nav_reset(ScreenID::HOME);
    state.startSession(25, 5);
    pump(100);
    advance_seconds(8 * 60 + 42);
    shot("03_focus", "03 Focus, timer hidden");
    focus_next();
    shot("03_focus_end", "03 Focus, knob on End session");
    focus_next();

    state.peek();
    shot("03b_peek", "03b Peek (lit for 5 s)");
    pump(5200);

    Session::instance().setDrift(true, "YouTube");
    advance_seconds(31);
    shot("04_drift", "04 Drift: YouTube for 0:31");
    Session::instance().setDrift(false, nullptr);
    advance_seconds(1);

    sim_encoder_hold();
    pump(50);
    shot("03_focus_confirm", "03 Focus, after a 2 s hold");
    click_focused(); // Keep going
    pump(100);

    // Run out the study session -> Intermission
    const SessionSnapshot snap = Session::instance().snapshot();
    advance_seconds(snap.remaining_s);
    advance_seconds(48);
    shot("05_intermission", "05 Intermission");

    Session::instance().setXp(7200); // Lv 15: goggles, bow tie, top hat
    nav_reset(ScreenID::HOME);
    shot("01_home_lv15", "01 Home at Lv 15 (all accessories)");

    pet_sheet(false);
    shot("pets_finishes", "Pet: 3 bodies x 4 finishes");
    pet_sheet(true);
    shot("pets_moods", "Pet: the 6 moods");

    fclose(s_index);
    return 0;
}
