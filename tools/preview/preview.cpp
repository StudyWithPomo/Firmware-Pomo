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

// Tap the first clickable pet on screen, like a finger would (PRESSED is what pets react to)
static lv_obj_t *find_pet(lv_obj_t *obj)
{
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE) && lv_obj_get_child_count(obj) == 0 &&
        lv_obj_get_width(obj) >= 32 && lv_obj_get_width(obj) % 16 == 0 &&
        lv_obj_get_width(obj) == lv_obj_get_height(obj)) return obj;
    for (uint32_t i = 0; i < lv_obj_get_child_count(obj); i++) {
        lv_obj_t *found = find_pet(lv_obj_get_child(obj, i));
        if (found) return found;
    }
    return nullptr;
}

static void tap_pet()
{
    lv_obj_t *pet = find_pet(ui_get_content_area());
    if (pet) lv_obj_send_event(pet, LV_EVENT_PRESSED, nullptr);
    else printf("tap_pet: no pet on screen\n");
}

// A stroke, as a finger dragging across the pet would make (the preview has no pointer)
static void stroke_pet()
{
    lv_obj_t *pet = find_pet(ui_get_content_area());
    if (pet) theme::pet_stroke(pet, lv_obj_get_parent(pet));
    else printf("stroke_pet: no pet on screen\n");
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
    static const char *const mood_names[] = {"happy", "love", "focus", "annoyed", "shh", "sleep", "sad", "hungry"};

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

static unsigned lvgl_used()
{
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    return (unsigned)(mon.total_size - mon.free_size);
}

// POMO_SOAK=N: go round every screen and flow N times and print LVGL memory at Home, to
// catch leaks (memory that keeps growing)
static void soak(int rounds)
{
    AppState &state = AppState::instance();
    lv_subject_set_int(&state.test_timing, 1);
    for (int i = 1; i <= rounds; i++) {
        nav_reset(ScreenID::HOME);
        tap_pet();
        pump(1200);
        nav_go(ScreenID::STUDY);
        click_focused();
        send_key(LV_KEY_RIGHT);
        send_key(LV_KEY_LEFT);
        click_focused();
        Navigator::instance().back();
        pump(50);
        nav_go(ScreenID::PET);
        tap_pet();
        pump(1200);
        nav_go(ScreenID::STATUS);
        Navigator::instance().back();
        pump(50);
        nav_go(ScreenID::WARDROBE);
        Navigator::instance().back();
        pump(50);
        nav_go(ScreenID::PLAYGROUND);
        for (int k = 0; k < 12; k++) send_key(LV_KEY_RIGHT); // a full wind: it dances
        pump(3200);
        click_focused(); // a gear
        pump(900);
        tap_pet();
        stroke_pet();
        pump(1200);
        nav_reset(ScreenID::HOME);
        nav_go(ScreenID::CIRCLE);

        // A test session: peek, drift, the confirm dialog, then break, ready, next, end set
        state.startSession();
        pump(100);
        state.peek();
        advance_seconds(6);
        Session::instance().laptopHeard(true);  // the laptop goes silent: paused, then alone
        advance_seconds(8);
        state.goOnAlone();
        pump(100);
        Session::instance().setDrift(true, "YouTube");
        advance_seconds(4);
        Session::instance().setDrift(false, nullptr);
        sim_encoder_hold();
        pump(50);
        click_focused(); // Keep going
        pump(100);
        advance_seconds(25);
        tap_pet();
        pump(1200);
        advance_seconds(30);
        state.startNextSession();
        pump(100);
        advance_seconds(30);
        advance_seconds(30);
        state.endSet();
        pump(100);

        nav_reset(ScreenID::HOME);
        pump(2000);
        if (i == 1 || i == 10 || i % 50 == 0 || i == rounds)
            printf("soak round %4d: LVGL heap used %u at Home\n", i, lvgl_used());
    }
    lv_subject_set_int(&state.test_timing, 0);
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
    Session::instance().laptopHeard(true);
    pump(100);
    shot("01_home_laptop", "01 Home, the laptop coupled (heartbeat coming in)");
    state.goOnAlone();
    pump(1100);

    AppState::CircleView solo = {};
    solo.can_invite = true;
    AppState::Friend friends[] = {
        {"Ava", true, 0, {1}, FriendStatus::FREE, false},
        {"Dev", true, 82, {2}, FriendStatus::FREE, false},
        {"Kai", true, 0, {3}, FriendStatus::BUSY, true},
    };
    state.setFriends(friends, 3, solo);
    pump(100);
    shot("01_home_circle", "01 Home, with Ava, Dev and Kai nearby");
    focus_next();
    shot("01_home_circle_focus", "01 Home, knob on the Study tile");
    tap_pet();
    shot("01_home_petted", "01 Home, pet tapped: hearts on their way up");
    pump(2100);
    stroke_pet();
    pump(60);
    shot("01_home_stroked", "01 Home, pet stroked: a squish and one heart");
    pump(2100);

    nav_reset(ScreenID::BOOT);
    click_focused(); // Yes, wind it
    shot("02_study", "02 Study (25 / 5), empty log");
    click_focused(); // grab Study
    send_key(LV_KEY_RIGHT);
    shot("02_study_45", "02 Study, length grabbed and turned to 45 min");
    send_key(LV_KEY_LEFT);
    click_focused(); // let go

    // Test timing: the chip in the Study header, then a 30 s session and break
    lv_subject_set_int(&state.test_timing, 1);
    pump(50);
    shot("02_study_test", "02 Study, test timing on");
    nav_reset(ScreenID::HOME);
    shot("01_home_test", "01 Home, test timing on");
    state.startSession();
    pump(100);
    advance_seconds(12);
    shot("03_focus_test", "03 Focus, a 30 s test session");
    advance_seconds(18 + 5);
    shot("05_intermission_test", "05 Intermission after a test session");
    advance_seconds(25);
    state.endSet(); // Finish for now on Ready
    pump(100);
    lv_subject_set_int(&state.test_timing, 0);

    nav_reset(ScreenID::HOME);
    nav_go(ScreenID::PET);
    shot("06_pet", "Pet hub");

    nav_go(ScreenID::STATUS);
    shot("06_status", "Pet status");
    Navigator::instance().back();
    pump(50);

    nav_go(ScreenID::PLAYGROUND);
    shot("06_playground", "Playground");
    for (int k = 0; k < 7; k++) send_key(LV_KEY_RIGHT);
    shot("06_playground_winding", "Playground, wound 7 of 12 clicks");
    for (int k = 0; k < 5; k++) send_key(LV_KEY_RIGHT);
    pump(700);
    shot("06_playground_dance", "Playground, fully wound: dancing (mid-spin)");
    pump(2600);
    shot("06_playground_after", "Playground, after the dance");
    pump(1600);
    click_focused(); // a knob press feeds a gear
    shot("06_playground_gear", "Playground, a gear in the air");
    pump(800);
    shot("06_playground_caught", "Playground, gear caught");
    pump(21000);
    shot("06_playground_asleep", "Playground, nobody playing for 20 s");
    state.setPet(PET_BODY_CAT, PET_FACE_PLAIN, PET_FINISH_VERDIGRIS);
    pump(50);
    shot("06_playground_cat", "Playground, a cat gets a key too");
    state.setPet(PET_BODY_BOT, PET_FACE_PLAIN, PET_FINISH_BRASS);
    Navigator::instance().back();
    pump(50);

    nav_go(ScreenID::WARDROBE);
    state.setPet(PET_BODY_CAT, PET_FACE_MONOCLE, PET_FINISH_COPPER);
    pump(50);
    shot("07_wardrobe", "07 Wardrobe (copper cat with monocle)");
    click_focused(); // enter the Body row
    shot("07_wardrobe_edit", "07 Wardrobe, editing the Body row");
    click_focused();
    state.setPet(PET_BODY_BOT, PET_FACE_PLAIN, PET_FINISH_BRASS);

    nav_reset(ScreenID::HOME);
    nav_go(ScreenID::CIRCLE);
    shot("08_circle", "Study circle: tap a friend to invite them");
    click_focused(); // Ava: choose the lengths first
    shot("08_circle_invite_study", "Study, opened to invite Ava");
    Navigator::instance().back();
    pump(50);

    AppState::CircleView hosting = {};
    hosting.role = GroupRole::HOSTING;
    hosting.study = 25;
    hosting.brk = 5;
    hosting.can_invite = true;
    friends[0].status = FriendStatus::INVITED;
    friends[1].status = FriendStatus::DECLINED;
    state.setFriends(friends, 3, hosting);
    shot("08_circle_hosting", "Study circle: Ava invited, Dev declined");
    friends[0].status = FriendStatus::MEMBER;
    friends[1].status = FriendStatus::FREE;
    hosting.can_start = true;
    state.setFriends(friends, 3, hosting);
    shot("08_circle_joined", "Study circle: Ava joined, Start or invite Dev too");

    // An invite arriving opens the Invite screen
    nav_reset(ScreenID::HOME);
    pump(50);
    AppState::CircleView invited = {};
    invited.role = GroupRole::INVITED;
    strcpy(invited.host, "Ibrahim");
    invited.study = 25;
    invited.brk = 5;
    AppState::Friend host[] = { {"Ibrahim", true, 0, {9}, FriendStatus::BUSY, false} };
    state.setFriends(host, 1, invited);
    shot("09_invite", "09 Invite: Ibrahim invited you");
    AppState::CircleView joined = invited;
    joined.role = GroupRole::JOINED;
    host[0].status = FriendStatus::MEMBER;
    state.setFriends(host, 1, joined);
    shot("09_invite_joined", "09 Invite, accepted: waiting for Ibrahim to start");
    joined.test = true;
    state.setFriends(host, 1, joined);
    shot("09_invite_test", "09 Invite with test timing");
    state.setFriends(friends, 0, solo);   // the invite went: back Home
    pump(100);

    // Studying with Ava and Dev for the sessions below
    AppState::CircleView in_set = {};
    in_set.role = GroupRole::IN_SET;
    AppState::Friend group[] = {
        {"Ava", true, 0, {1}, FriendStatus::MEMBER, true},
        {"Dev", true, 0, {2}, FriendStatus::MEMBER, true},
        {"Kai", true, 0, {3}, FriendStatus::BUSY, true},
    };
    state.setFriends(group, 3, in_set);
    Session::instance().setCircle(2, false);
    pump(100);

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
    Session::instance().setDrift(true, "Idle", DriftKind::IDLE);
    advance_seconds(75);
    shot("04_drift_idle", "04 Drift: gone idle for 1:15");
    Session::instance().setDrift(false, nullptr);
    advance_seconds(1);

    // The dashboard's heartbeat stops: the timer is held until it's back (or Go on alone)
    Session::instance().laptopHeard(true);
    advance_seconds(8);
    shot("04b_laptop_lost", "04b Laptop lost: session paused");
    Session::instance().laptopHeard(true);
    pump(100);
    state.goOnAlone();
    pump(100);

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
    tap_pet();
    shot("05_intermission_hearts", "05 Intermission, pet tapped");
    pump(1500);

    // Run out the break -> Ready, then Finish for now -> Home
    advance_seconds(Session::instance().snapshot().remaining_s);
    shot("05b_ready", "05b Ready for session II of IV?");
    focus_next();
    click_focused();
    pump(100);

    nav_reset(ScreenID::HOME);
    nav_go(ScreenID::STUDY);
    shot("02_study_log", "02 Study, after one session");
    nav_go(ScreenID::STATUS);
    shot("06_status_log", "Pet status, after a session");

    // Ended early: the score in a notice over Home (the laptop prints the time card)
    nav_reset(ScreenID::HOME);
    state.startSession(25, 5);
    advance_seconds(4 * 60 + 5);
    state.endSessionEarly();
    pump(200);
    shot("01_home_ended_early", "01 Home, after ending a session early");
    pump(4200);

    // Reaching Lv 5 unlocks the goggles
    Session::instance().setXp(1990);
    state.startSession(25, 5);
    advance_seconds(25 * 60);
    pump(200);
    shot("05_intermission_levelup", "05 Intermission, the session reached Lv 5");
    pump(4200);
    state.endSet();
    pump(200);

    // Pet care: the sessions above earned gears; time makes the pet peckish, then hungry
    state.update(); // the clock is known: the hunger clock starts now
    nav_reset(ScreenID::HOME);
    shot("01_home_gears", "01 Home, the gears bar after two completed sessions");
    g_sim_epoch += 13 * 3600;
    state.update();
    nav_reset(ScreenID::BOOT);
    shot("00_boot_peckish", "00 Boot, 13 h unfed: peckish");
    g_sim_epoch += 12 * 3600;
    state.update();
    nav_reset(ScreenID::BOOT);
    shot("00_boot_hungry", "00 Boot, 25 h unfed: hungry");
    nav_reset(ScreenID::HOME);
    shot("01_home_hungry", "01 Home, hungry");
    nav_go(ScreenID::PLAYGROUND);
    shot("06_playground_hungry", "Playground, hungry");
    click_focused(); // feed a gear
    pump(800);
    state.update();
    shot("06_playground_fed", "Playground, a hungry pet fed");
    click_focused(); // and another
    pump(800);
    state.update();
    click_focused(); // none left
    pump(100);
    shot("06_playground_empty", "Playground, out of gears");
    nav_reset(ScreenID::HOME);
    shot("01_home_full", "01 Home, just fed: full");
    nav_go(ScreenID::STATUS);
    shot("06_status_fed", "Pet status, last fed");
    Navigator::instance().back();
    pump(50);

    Session::instance().setXp(7200); // Lv 15: goggles, bow tie, top hat
    nav_reset(ScreenID::HOME);
    shot("01_home_lv15", "01 Home at Lv 15 (all accessories)");

    pet_sheet(false);
    shot("pets_finishes", "Pet: 3 bodies x 4 finishes");
    pet_sheet(true);
    shot("pets_moods", "Pet: the 8 moods");

    fclose(s_index);
    if (const char *rounds = getenv("POMO_SOAK")) soak(atoi(rounds));
    return 0;
}
